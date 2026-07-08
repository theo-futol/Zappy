"""End-to-end tests against the real C++ Zappy server (compose `server` service).

Skipped unless ZAPPY_E2E_HOST / ZAPPY_E2E_PORT point at a running server.
Start it with a generous client count — every connection consumes an egg and
this suite opens ~8 per team:

    docker compose build server && ZAPPY_CLIENTS=20 docker compose up -d server
    docker run --rm --network bonus_zappy-net -v $PWD/ZapLauncher:/app -w /app \
        -e API_KEY=test -e ZAPPY_E2E_HOST=server -e ZAPPY_E2E_PORT=4242 \
        python:trixie bash -c "pip install -q -r requirements-dev.txt && pytest tests/test_e2e_real_server.py"

The server runs in serial-turn mode (--wait-timeout); replies can lag by a
turn, so every read is given generous patience.
"""

import os
import socket as _socket
import time

import pytest

from multiagent import ServerConnection, GraphicConnection, GuiMonitor

HOST = os.getenv("ZAPPY_E2E_HOST")
PORT = os.getenv("ZAPPY_E2E_PORT")

pytestmark = pytest.mark.skipif(
    not (HOST and PORT),
    reason="set ZAPPY_E2E_HOST and ZAPPY_E2E_PORT to run against a real server",
)


def connect(team="REDS") -> ServerConnection:
    return ServerConnection(HOST, int(PORT), team)


def collect_gui_lines(conn: GraphicConnection, seconds: float) -> list[str]:
    """Read every line the server pushes to a graphic client for `seconds`."""
    lines = []
    conn._sock.settimeout(0.3)
    deadline = time.time() + seconds
    try:
        while time.time() < deadline:
            try:
                lines.append(conn._recv_line())
            except _socket.timeout:
                continue
    finally:
        conn._sock.settimeout(None)
    return lines


def parse_tile0(raw_look: str) -> str:
    inner = raw_look.strip().lstrip("[").rstrip("]")
    return inner.split(",")[0]


def test_handshake_and_movement():
    conn = connect()
    assert int(conn.slots) >= 1
    width, height = conn.map_size.split()
    assert int(width) > 0 and int(height) > 0
    assert conn.send_command("Forward") == "ok"
    assert conn.send_command("Left") == "ok"
    assert conn.send_command("Right") == "ok"
    conn.close()


def test_look_and_inventory_formats():
    conn = connect()
    look = conn.send_command("Look")
    assert look.startswith("[") and look.endswith("]")
    inv = conn.send_command("Inventory")
    assert "food" in inv
    conn.close()


def test_take_absent_resource_is_ko():
    """Regression: Take used to answer ok (and notify the GUI) for a resource
    that was not on the tile at all."""
    conn = connect()
    tile0 = parse_tile0(conn.send_command("Look"))
    for stone in ("thystame", "phiras", "mendiane", "deraumere", "sibur", "linemate"):
        present = f"{stone}:0" not in tile0 and stone in tile0
        if not present:
            assert conn.send_command(f"Take {stone}") == "ko"
            break
    else:
        pytest.skip("tile unexpectedly holds every stone type")
    conn.close()


def test_set_without_item_is_ko():
    conn = connect()
    assert conn.send_command("Set thystame") == "ko"
    conn.close()


def test_broadcast_reaches_other_players():
    sender = connect()
    listener = connect()
    assert sender.send_command("Broadcast e2e_ping") == "ok"
    # the listener sees the message on its next command round-trip
    deadline = time.time() + 10
    heard = []
    while time.time() < deadline and not heard:
        listener.send_command("Inventory")
        heard = [m for m in listener.drain_messages() if "e2e ping" in m]
    assert heard, "broadcast never reached the second player"
    sender.close()
    listener.close()


def test_fork_emits_egg_events_to_gui():
    """Regression: Fork used to lay eggs silently; the GUI now gets pfk + enw."""
    gui = GraphicConnection(HOST, int(PORT))
    player = connect()
    assert player.send_command("Fork") == "ok"
    lines = collect_gui_lines(gui, 5)
    assert any(line.startswith("pfk ") for line in lines), lines
    assert any(line.startswith("enw ") for line in lines), lines
    player.close()
    gui.close()


def parse_inventory(raw: str) -> dict[str, int]:
    """'[food 10, linemate 0, ...]' -> {'food': 10, 'linemate': 0, ...}"""
    inner = raw.strip().lstrip("[").rstrip("]")
    inv = {}
    for pair in inner.split(","):
        parts = pair.split()
        if len(parts) == 2:
            inv[parts[0]] = int(parts[1])
    return inv


def gui_query(conn: GraphicConnection, cmd: str, prefix: str, timeout: float = 5.0) -> str:
    """Send a GUI command and return the reply line, skipping passive events."""
    conn._send(cmd + "\n")
    conn._sock.settimeout(0.3)
    deadline = time.time() + timeout
    try:
        while time.time() < deadline:
            try:
                line = conn._recv_line()
            except _socket.timeout:
                continue
            if line.startswith(prefix):
                return line
    finally:
        conn._sock.settimeout(None)
    raise AssertionError(f"no '{prefix}' reply to '{cmd}' within {timeout}s")


def test_agent_and_gui_views_agree_on_inventory_and_level():
    """Consistency across layers: the resources/level the AI sees must match
    what the GUI protocol reports for the same player."""
    gui = GraphicConnection(HOST, int(PORT))
    agent = connect()

    # identify our player id from the pnw event the GUI receives
    lines = collect_gui_lines(gui, 5)
    pnw = [line for line in lines if line.startswith("pnw ")]
    assert pnw, f"GUI never saw the new player: {lines}"
    player_id = pnw[-1].split()[1].lstrip("#")

    # change the inventory if the world allows it (food on a walkable tile)
    for _ in range(10):
        if agent.send_command("Take food") == "ok":
            break
        agent.send_command("Forward")

    ai_inv = parse_inventory(agent.send_command("Inventory"))
    assert ai_inv, "agent inventory did not parse"

    # GUI view of the same player: pin <id> <x> <y> q0..q6
    pin = gui_query(gui, f"pin {player_id}", "pin ").split()
    gui_counts = [int(v) for v in pin[4:11]]
    resources = ["food", "linemate", "deraumere", "sibur", "mendiane", "phiras", "thystame"]
    for name, gui_count in zip(resources, gui_counts):
        assert ai_inv[name] == gui_count, (
            f"{name}: agent sees {ai_inv[name]}, GUI sees {gui_count}"
        )

    # GUI view of the level: plv <id> <level> — a fresh player is level 1
    plv = gui_query(gui, f"plv {player_id}", "plv ").split()
    assert int(plv[2]) == 1

    # GUI position matches what movement implies (position is on the map)
    ppo = gui_query(gui, f"ppo {player_id}", "ppo ").split()
    width, height = (int(v) for v in agent.map_size.split())
    assert 0 <= int(ppo[2]) < width and 0 <= int(ppo[3]) < height

    agent.close()
    gui.close()


def test_gui_monitor_claims_and_mirrors_real_server():
    """The GuiMonitor must map a fresh agent to its player id and mirror its
    authoritative facing/level/inventory from the GUI event stream."""
    monitor = GuiMonitor(HOST, int(PORT))
    monitor.start()
    agent = connect()
    try:
        player_id = monitor.claim_player("REDS", timeout=5.0)
        assert player_id is not None, "monitor never saw the pnw for the new agent"

        deadline = time.time() + 5
        player = None
        while time.time() < deadline:
            player = monitor.get_player(player_id)
            if player and player.get("inventory"):
                break
            time.sleep(0.05)
        assert player, "player never appeared in the monitor registry"
        assert player["orientation"] in (1, 2, 3, 4)
        assert player["level"] == 1
        assert player["team"] == "REDS"
        # the connect-time pipi carries the starting inventory
        assert player["inventory"]["food"] == 10

        # the mirror must match the agent's own view
        ai_inv = parse_inventory(agent.send_command("Inventory"))
        assert ai_inv == player["inventory"]
    finally:
        agent.close()
        monitor.close()


def test_connection_hatches_egg_and_emits_ebo():
    """A new player consumes an egg: the GUI must see ebo + pnw."""
    gui = GraphicConnection(HOST, int(PORT))
    newcomer = connect()
    lines = collect_gui_lines(gui, 5)
    assert any(line.startswith("ebo ") for line in lines), lines
    assert any(line.startswith("pnw ") for line in lines), lines
    newcomer.close()
    gui.close()
