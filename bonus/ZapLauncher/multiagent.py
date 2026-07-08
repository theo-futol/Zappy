import os
import random
import select as _select
import socket as _socket
import threading
import time
from typing import Literal, NotRequired, TypedDict
from dataclasses import dataclass, field

PROMPT_DATABASE_DIR = os.path.join(os.path.dirname(__file__), "prompt_database")
TEAMS_PROMPT_DIR = os.path.join(PROMPT_DATABASE_DIR, "teams")
PERSONALITIES_PROMPT_DIR = os.path.join(PROMPT_DATABASE_DIR, "personalities")
TRANTORIAN_PROMPT_PATH = os.path.join(PROMPT_DATABASE_DIR, "TRANTORIAN.md")
PROTOCOL_PROMPT_PATH = os.path.join(PROMPT_DATABASE_DIR, "protocol.txt")

DEFAULT_MODEL = "mistral-small-latest"
DEFAULT_ENDPOINT = "https://api.mistral.ai/v1"

_protocol_cache: str | None = None
server_connected: bool = False


class _BaseConnection:
    """Shared socket plumbing for server connections."""

    def __init__(self, host: str, port: int):
        self._sock = _socket.socket(_socket.AF_INET, _socket.SOCK_STREAM)
        self._sock.connect((host, port))
        self._buf = b""

    def close(self):
        try:
            self._sock.close()
        except Exception:
            pass

    def __del__(self):
        self.close()

    def _send(self, data: str):
        self._sock.sendall(data.encode("utf-8"))

    def _recv_line(self) -> str:
        while b"\n" not in self._buf:
            chunk = self._sock.recv(4096)
            if not chunk:
                raise ConnectionError("Server closed connection")
            self._buf += chunk
        idx = self._buf.index(b"\n")
        line = self._buf[:idx].decode("utf-8").strip()
        self._buf = self._buf[idx + 1:]
        return line


class ServerConnection(_BaseConnection):
    """TCP connection from a ZapLauncher agent to the Zappy game server.

    Performs the AI client handshake on construction:
      server → WELCOME
      client → <team_name>
      server → <available_slots>
      server → <width> <height>

    After that, send_command() sends one protocol command and returns
    the server's reply, discarding any interleaved broadcast messages.
    """

    def __init__(self, host: str, port: int, team: str):
        super().__init__(host, port)
        self._dead = False
        self._pending_messages: list[str] = []
        self._recv_line()                    # WELCOME
        self._send(team + "\n")
        slots_line = self._recv_line()       # available slot count — or "ko"
        if slots_line == "ko":
            self.close()
            raise ConnectionError(
                f"Server rejected team '{team}': no available slots"
            )
        self.slots = slots_line
        self.map_size = self._recv_line()    # "width height"

    def send_command(self, cmd: str) -> str:
        if self._dead:
            raise ConnectionError("Player is dead")
        self._send(cmd + "\n")
        while True:
            line = self._recv_line()
            if line == "dead":
                self._dead = True
                self.close()
                raise ConnectionError("Player is dead")
            if line.startswith("waiting "):
                continue
            if line.startswith("message "):
                # Queue for delivery to the agent; decode underscores back to spaces
                self._pending_messages.append(line.replace("_", " "))
                continue
            return line

    def drain_messages(self) -> list[str]:
        """Return and clear all broadcast messages received since the last drain."""
        msgs = self._pending_messages[:]
        self._pending_messages.clear()
        return msgs

    def poll_waiting(self) -> int | None:
        """Non-blocking: returns timeout_ms if a 'waiting N' notification is pending.

        Checks the internal read buffer first (data already pulled off the socket
        by a previous _recv_line call), then peeks the socket without consuming
        any bytes.  Returns None if no notification is pending or the connection
        is dead / in error.
        """
        if self._dead:
            return None
        try:
            check_buf = self._buf
            if b"\n" not in check_buf:
                readable, _, _ = _select.select([self._sock], [], [], 0)
                if not readable:
                    return None
                peeked = self._sock.recv(4096, _socket.MSG_PEEK)
                if not peeked:
                    self._dead = True
                    return None
                check_buf = self._buf + peeked
            if b"\n" not in check_buf:
                return None
            first_line = check_buf[:check_buf.index(b"\n")].strip()
            if first_line.startswith(b"waiting "):
                return int(first_line[8:])
        except Exception:
            pass
        return None

    def get_available_slots(self) -> int | None:
        try:
            return int(self.send_command("Connect_nbr"))
        except Exception:
            return None

    def is_alive(self) -> bool:
        if self._dead:
            return False
        try:
            readable, _, _ = _select.select([self._sock], [], [], 0)
            if readable:
                peeked = self._sock.recv(4096, _socket.MSG_PEEK)
                if not peeked:
                    return False
                combined = self._buf + peeked
            else:
                combined = self._buf
            for line in combined.split(b"\n"):
                if line.strip() == b"dead":
                    self._dead = True
                    return False
            return True
        except Exception:
            return False


class GraphicConnection(_BaseConnection):
    """Graphic client connection used for server metadata queries (e.g. team names).

    Handshake:
      server → WELCOME
      client → GRAPHIC
      (server sets client type silently; no acknowledgement is sent)
    """

    def __init__(self, host: str, port: int):
        super().__init__(host, port)
        self._recv_line()       # WELCOME
        self._send("GRAPHIC\n")

    def get_team_names(self) -> list[str]:
        """Send tna and collect all returned team names.

        The server sends one 'tna <name>' line per team in a single burst.
        We stop reading after 0.5 s of silence to detect the end of the list.
        """
        self._send("tna\n")
        names: list[str] = []
        self._sock.settimeout(0.5)
        try:
            while True:
                try:
                    line = self._recv_line()
                    if line.startswith("tna "):
                        names.append(line[4:].strip())
                except _socket.timeout:
                    break
        finally:
            self._sock.settimeout(None)
        return names


RESOURCE_NAMES = ["food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"]

FACING_NAMES = {1: "north", 2: "east", 3: "south", 4: "west"}


class GuiMonitor(threading.Thread):
    """Background GRAPHIC client mirroring authoritative per-player state.

    The AI protocol never tells a client its own player id, absolute position
    or facing. This monitor watches the server's GUI event stream (pnw, pipi,
    ppo, plv, pin, pdi) and keeps a registry so the launcher can map each
    agent to its player id right after connection (claim_player) and expose
    absolute state — facing, level, inventory — e.g. to the player POV page.
    """

    def __init__(self, host: str, port: int):
        super().__init__(daemon=True)
        self._conn = GraphicConnection(host, port)
        self._lock = threading.Lock()
        self.players: dict[int, dict] = {}
        self._unclaimed: list[tuple[int, str]] = []
        self.alive = True

    @staticmethod
    def _pid(token: str) -> int:
        return int(token.lstrip("#"))

    def run(self):
        try:
            while True:
                self._handle(self._conn._recv_line())
        except Exception:
            self.alive = False

    def _handle(self, line: str) -> None:
        parts = line.split()
        if not parts:
            return
        cmd = parts[0]
        try:
            with self._lock:
                if cmd == "pnw" and len(parts) >= 7:
                    pid = self._pid(parts[1])
                    self.players[pid] = {
                        "x": int(parts[2]), "y": int(parts[3]),
                        "orientation": int(parts[4]), "level": int(parts[5]),
                        "team": parts[6], "inventory": None,
                    }
                    self._unclaimed.append((pid, parts[6]))
                elif cmd == "pipi" and len(parts) >= 13:
                    pid = self._pid(parts[1])
                    player = self.players.setdefault(pid, {})
                    player.update(
                        x=int(parts[2]), y=int(parts[3]),
                        orientation=int(parts[4]), level=int(parts[5]),
                        inventory=dict(zip(RESOURCE_NAMES, (int(v) for v in parts[6:13]))),
                    )
                elif cmd == "ppo" and len(parts) >= 5:
                    pid = self._pid(parts[1])
                    self.players.setdefault(pid, {}).update(
                        x=int(parts[2]), y=int(parts[3]), orientation=int(parts[4]),
                    )
                elif cmd == "plv" and len(parts) >= 3:
                    self.players.setdefault(self._pid(parts[1]), {})["level"] = int(parts[2])
                elif cmd == "pin" and len(parts) >= 11:
                    pid = self._pid(parts[1])
                    self.players.setdefault(pid, {})["inventory"] = dict(
                        zip(RESOURCE_NAMES, (int(v) for v in parts[4:11]))
                    )
                elif cmd == "pdi" and len(parts) >= 2:
                    self.players.pop(self._pid(parts[1]), None)
        except (ValueError, IndexError):
            pass

    def claim_player(self, team: str, timeout: float = 3.0) -> int | None:
        """Return the player id of the most recent unclaimed connection for `team`.

        Called right after an agent's handshake: the server pushes the matching
        pnw within its next loop iteration, so we poll briefly for it.
        """
        deadline = time.time() + timeout
        while time.time() < deadline:
            with self._lock:
                for i, (pid, pnw_team) in enumerate(self._unclaimed):
                    if pnw_team == team:
                        self._unclaimed.pop(i)
                        return pid
            time.sleep(0.02)
        return None

    def get_player(self, player_id: int) -> dict | None:
        with self._lock:
            player = self.players.get(player_id)
            return dict(player) if player else None

    def close(self):
        self.alive = False
        self._conn.close()


class Team(TypedDict):
    """
        Informations shared between same-team agents, and which LLM to use.
    """
    id: str
    shared_prompt: str

    model: str
    endpoint: str
    prompt_cache_key: str

    initial_slots: NotRequired[int]  # total configured slots; set on first agent connection


class Message(TypedDict):
    """
        A single message sent to the LLM.
    """
    role: Literal["system", "user", "assistant", "tool"]
    content: str


class HistoryEntry(TypedDict, total=False):
    """
        One entry of an agent's conversation log, exposed by the API.
        `kind` lets a dashboard separate turns without parsing text:
        - "user": a message sent to the agent.
        - "llm_response": one full raw LLM answer (may contain several USE lines).
        - "tool_use": the agent chose to USE `tool_name`.
        - "tool_result": the result fed back from that tool use.
        - "report": the agent's final REPORT text for the turn.
    """
    kind: Literal["user", "llm_response", "tool_use", "tool_result", "report"]
    content: str
    tool_name: str
    tool_input: str  # full "USE" payload: tool name + arguments
    message_id: str
    malformed: bool
    truncated: bool
    timestamp: float  # unix epoch seconds; set when the entry is created


@dataclass
class Agent:
    id: str
    team_id: str

    # The cacheable prefix: protocol + Trantorian lore + team shared prompt.
    cached_prompt: str

    # The agent-specific instructions (its "personality").
    personality_prompt: str

    # Live TCP connection to the Zappy server (None when no server is reachable).
    connection: ServerConnection | None = None

    # Server-side player id (from the GUI event stream), when a GuiMonitor is running.
    player_id: int | None = None

    facts_memory: list[str] = field(default_factory=list)
    history: list[HistoryEntry] = field(default_factory=list)

    def __getitem__(self, key: str):
        return getattr(self, key)

    def __setitem__(self, key: str, value):
        setattr(self, key, value)


def list_team_ids() -> list[str]:
    return [
        os.path.splitext(name)[0]
        for name in os.listdir(TEAMS_PROMPT_DIR)
        if name.endswith(".txt")
    ]


def list_personality_names() -> list[str]:
    return [
        os.path.splitext(name)[0]
        for name in os.listdir(PERSONALITIES_PROMPT_DIR)
        if name.endswith(".txt")
    ]


def get_team_prompt_content(team_id: str) -> str:
    """
        Read a team's shared prompt straight from
        prompt_database/teams/<team_id>.txt.
    """
    path = os.path.join(TEAMS_PROMPT_DIR, f"{team_id}.txt")
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def save_team_file(team_id: str, shared_prompt: str) -> None:
    """
        Persist a team's shared prompt to prompt_database/teams/<team_id>.txt.
    """
    path = os.path.join(TEAMS_PROMPT_DIR, f"{team_id}.txt")
    with open(path, "w", encoding="utf-8") as f:
        f.write(shared_prompt)


def load_personality_prompt(personality: str) -> str:
    """
        Read a personality prompt from
        prompt_database/personalities/<personality>.txt.
    """
    path = os.path.join(PERSONALITIES_PROMPT_DIR, f"{personality}.txt")
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def save_personality_file(personality: str, content: str) -> None:
    """
        Persist a personality prompt to
        prompt_database/personalities/<personality>.txt.
    """
    path = os.path.join(PERSONALITIES_PROMPT_DIR, f"{personality}.txt")
    with open(path, "w", encoding="utf-8") as f:
        f.write(content)


def load_protocol_prompt() -> str:
    global _protocol_cache
    if _protocol_cache is None:
        with open(PROTOCOL_PROMPT_PATH, "r", encoding="utf-8") as f:
            _protocol_cache = f.read()
    return _protocol_cache


def save_protocol_file(content: str) -> None:
    global _protocol_cache
    _protocol_cache = content
    with open(PROTOCOL_PROMPT_PATH, "w", encoding="utf-8") as f:
        f.write(content)


def load_trantorian_prompt() -> str:
    """
        Read the Trantorian lore from prompt_database/TRANTORIAN.md.
    """
    with open(TRANTORIAN_PROMPT_PATH, "r", encoding="utf-8") as f:
        return f.read()


def build_cached_prompt(team: Team) -> str:
    """
        Build the cacheable prefix shared by every agent of `team`: the
        USE/REPORT protocol, the Trantorian lore, then the team's shared
        prompt.
    """
    return "\n\n".join([
        load_protocol_prompt(),
        load_trantorian_prompt(),
        team["shared_prompt"],
    ])


def load_teams() -> dict[str, Team]:
    """
        Build a fully populated Team for every <team_id>.txt found in
        prompt_database/teams/, keyed by team_id.
    """
    teams: dict[str, Team] = {}

    for team_id in list_team_ids():
        teams[team_id] = Team(
            id=team_id,
            shared_prompt=get_team_prompt_content(team_id),
            model=DEFAULT_MODEL,
            endpoint=DEFAULT_ENDPOINT,
            prompt_cache_key=team_id,
        )

    return teams


def load_teams_from_server(host: str, port: int) -> dict[str, Team]:
    """
        Query the Zappy server for its team list via the GRAPHIC protocol,
        then build a Team object for each name.

        If a matching prompt_database/teams/<name>.txt file exists it is
        loaded as the shared prompt; otherwise the shared prompt is empty.
        Raises RuntimeError when the server returns no teams.
    """
    conn = GraphicConnection(host, port)
    team_names = conn.get_team_names()
    conn.close()

    if not team_names:
        raise RuntimeError("Server returned an empty team list")

    teams: dict[str, Team] = {}
    for name in team_names:
        try:
            shared_prompt = get_team_prompt_content(name)
        except FileNotFoundError:
            shared_prompt = ""
        teams[name] = Team(
            id=name,
            shared_prompt=shared_prompt,
            model=DEFAULT_MODEL,
            endpoint=DEFAULT_ENDPOINT,
            prompt_cache_key=name,
        )
    return teams


def create_agent(id: str, team: Team, personality: str) -> Agent:
    """
        Build an Agent for `team` using the named personality. Its prompt
        is split into a cacheable prefix (protocol + Trantorian lore +
        team shared prompt) and an agent-specific personality prompt.
        If ZAPPY_SERVER_HOST and ZAPPY_SERVER_PORT are set, opens a live
        TCP connection to the server under the team's name.
    """
    conn: ServerConnection | None = None
    host = os.getenv("ZAPPY_SERVER_HOST", "")
    port_str = os.getenv("ZAPPY_SERVER_PORT", "")
    if host and port_str:
        conn = ServerConnection(host, int(port_str), team["id"])
        if "initial_slots" not in team:
            team["initial_slots"] = int(conn.slots)
        print(f"[agent:{id}] Connected to server {host}:{port_str} as team '{team['id']}' (slots: {conn.slots}, map: {conn.map_size})")
    return Agent(
        id=id,
        team_id=team["id"],
        cached_prompt=build_cached_prompt(team),
        personality_prompt=load_personality_prompt(personality),
        connection=conn,
    )


def random_agent(team: Team, id: str | None = None) -> Agent:
    """
        Create a new Agent for `team` using a random personality picked
        from prompt_database/personalities/.
    """
    personality = random.choice(list_personality_names())

    return create_agent(id or personality, team, personality)
