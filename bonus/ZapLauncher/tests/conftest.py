"""
Shared fixtures: a fake Zappy TCP server speaking the AI/GRAPHIC protocol,
plus isolation of runtime state and the prompt database for every test.
"""

import os
import socket
import sys
import threading

# The engine module instantiates the OpenAI client at import time.
os.environ.setdefault("API_KEY", "test-key")

sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

import pytest  # noqa: E402


class FakeZappyServer(threading.Thread):
    """Minimal Zappy game server: handshake + canned command replies.

    - `responses[prefix]` overrides the reply for commands starting with
      prefix; the value may be a string or a list of lines (sent in order,
      letting tests interleave `message`/`waiting` lines before the reply).
    - `push_to_ai(line)` sends an unsolicited line to every AI client
      (e.g. a 'waiting 2000' turn notification or a broadcast).
    """

    DEFAULTS = {
        "Forward": "ok",
        "Left": "ok",
        "Right": "ok",
        "Look": "[player food:2 linemate:0, linemate:1 food:0, , food:1]",
        "Inventory": "[food 10, linemate 0, deraumere 0, sibur 0, mendiane 0, phiras 0, thystame 0]",
        "Broadcast": "ok",
        "Connect_nbr": "3",
        "Fork": "ok",
        "Eject": "ko",
        "Take": "ok",
        "Set": "ok",
        "Incantation": "Elevation underway",
    }

    def __init__(self, team_names=("REDS",), slots=3, map_size=(10, 10)):
        super().__init__(daemon=True)
        self.team_names = list(team_names)
        self.slots = slots
        self.map_size = map_size
        self.received: list[tuple[str, str]] = []
        self.responses: dict[str, object] = {}
        self._ai_conns: list[socket.socket] = []
        self._graphic_conns: list[socket.socket] = []
        self._next_player_id = 1
        self._lock = threading.Lock()
        self._srv = socket.create_server(("127.0.0.1", 0))
        self._srv.settimeout(0.2)
        self.port = self._srv.getsockname()[1]
        self._running = True

    # -- lifecycle ---------------------------------------------------------

    def stop(self):
        self._running = False
        try:
            self._srv.close()
        except OSError:
            pass
        with self._lock:
            for conn in self._ai_conns + self._graphic_conns:
                try:
                    conn.close()
                except OSError:
                    pass

    def run(self):
        while self._running:
            try:
                conn, _ = self._srv.accept()
            except socket.timeout:
                continue
            except OSError:
                break
            threading.Thread(target=self._handle, args=(conn,), daemon=True).start()

    # -- helpers -----------------------------------------------------------

    def push_to_ai(self, line: str):
        with self._lock:
            for conn in self._ai_conns:
                try:
                    conn.sendall((line + "\n").encode())
                except OSError:
                    pass

    def push_to_graphic(self, line: str):
        with self._lock:
            for conn in self._graphic_conns:
                try:
                    conn.sendall((line + "\n").encode())
                except OSError:
                    pass

    def _send(self, conn, line: str):
        conn.sendall((line + "\n").encode())

    def _readline(self, f) -> str | None:
        raw = f.readline()
        if not raw:
            return None
        return raw.decode().strip()

    def _reply_lines(self, cmd: str) -> list[str]:
        for prefix, reply in self.responses.items():
            if cmd.startswith(prefix):
                return list(reply) if isinstance(reply, list) else [reply]
        word = cmd.split()[0] if cmd.split() else cmd
        return [self.DEFAULTS.get(word, "ko")]

    def _handle(self, conn):
        f = conn.makefile("rb")
        try:
            self._send(conn, "WELCOME")
            team = self._readline(f)
            if team is None:
                return
            if team == "GRAPHIC":
                with self._lock:
                    self._graphic_conns.append(conn)
                while True:
                    cmd = self._readline(f)
                    if cmd is None:
                        return
                    self.received.append(("GRAPHIC", cmd))
                    if cmd == "tna":
                        for name in self.team_names:
                            self._send(conn, f"tna {name}")
            else:
                if team not in self.team_names or self.slots <= 0:
                    self._send(conn, "ko")
                    return
                self._send(conn, str(self.slots))
                self._send(conn, f"{self.map_size[0]} {self.map_size[1]}")
                with self._lock:
                    self._ai_conns.append(conn)
                    player_id = self._next_player_id
                    self._next_player_id += 1
                # Mirror the real server: announce the new player to graphic clients.
                self.push_to_graphic(f"pnw {player_id} 2 3 1 1 {team}")
                self.push_to_graphic(f"pipi {player_id} 2 3 1 1 10 0 0 0 0 0 0")
                while True:
                    cmd = self._readline(f)
                    if cmd is None:
                        return
                    self.received.append((team, cmd))
                    for line in self._reply_lines(cmd):
                        self._send(conn, line)
        except OSError:
            pass
        finally:
            with self._lock:
                if conn in self._ai_conns:
                    self._ai_conns.remove(conn)
            try:
                conn.close()
            except OSError:
                pass


@pytest.fixture
def fake_server():
    srv = FakeZappyServer()
    srv.start()
    yield srv
    srv.stop()


@pytest.fixture
def server_env(fake_server, monkeypatch):
    """Point agent creation at the fake server."""
    monkeypatch.setenv("ZAPPY_SERVER_HOST", "127.0.0.1")
    monkeypatch.setenv("ZAPPY_SERVER_PORT", str(fake_server.port))
    return fake_server


@pytest.fixture
def no_server_env(monkeypatch):
    """Ensure agents are created without a server connection."""
    monkeypatch.delenv("ZAPPY_SERVER_HOST", raising=False)
    monkeypatch.delenv("ZAPPY_SERVER_PORT", raising=False)


@pytest.fixture
def clean_runtime(monkeypatch, tmp_path):
    """Fresh agent registry, busy/manual sets, GUI monitor, and playbooks dir."""
    import runtime
    import api

    monkeypatch.setattr(runtime, "agents", {})
    monkeypatch.setattr(runtime, "_gui_monitor", None)
    api._busy_agents.clear()
    api._manual_agents.clear()
    playbooks_dir = tmp_path / "playbooks"
    playbooks_dir.mkdir()
    monkeypatch.setattr(api, "PLAYBOOKS_DIR", str(playbooks_dir))
    yield runtime
    monitor = runtime._gui_monitor
    if monitor is not None:
        monitor.close()
    runtime._gui_monitor = None
    api._busy_agents.clear()
    api._manual_agents.clear()
