"""
    Shared in-memory state for the current run: the teams and agents that
    are "running" right now, plus a log of token usage per LLM call.
    Every API entry point (routes, autonomous loop, playbook runner) reads
    and mutates this module's state, so they always reflect the same loop.
"""

import os
import multiagent
from multiagent import Team, Agent, load_teams

_server_host = os.getenv("ZAPPY_SERVER_HOST", "")
_server_port = os.getenv("ZAPPY_SERVER_PORT", "")
_clients_per_team = int(os.getenv("ZAPPY_CLIENTS", "0")) or None

# Load teams: the server is the authoritative source; local files are the fallback.
if _server_host and _server_port:
    try:
        teams: dict[str, Team] = multiagent.load_teams_from_server(_server_host, int(_server_port))
        multiagent.server_connected = True
        print(f"[runtime] Teams loaded from server: {list(teams.keys())}")
    except Exception as exc:
        print(f"[runtime] Warning: could not load teams from server: {exc}. Falling back to local files.")
        teams = load_teams()

    for _team in teams.values():
        _team["initial_slots"] = _clients_per_team  # type: ignore[literal-required]
        print(f"[runtime] Team '{_team['id']}': {_clients_per_team or '?'} total slots")
else:
    teams = load_teams()

agents: dict[str, Agent] = {}
usage_log: dict[str, dict] = {}

# Lazily created GUI monitor (GRAPHIC connection mirroring player state).
_gui_monitor: multiagent.GuiMonitor | None = None


def get_gui_monitor() -> multiagent.GuiMonitor | None:
    """The process-wide GuiMonitor, created on first use; None without a server.

    Reads the env at call time (not import time) and recreates the monitor if
    the target changed or the connection died.
    """
    global _gui_monitor
    host = os.getenv("ZAPPY_SERVER_HOST", "")
    port = os.getenv("ZAPPY_SERVER_PORT", "")
    if not host or not port:
        return None
    key = (host, port)
    if _gui_monitor is not None and _gui_monitor.alive and getattr(_gui_monitor, "key", None) == key:
        return _gui_monitor
    try:
        monitor = multiagent.GuiMonitor(host, int(port))
        monitor.key = key
        monitor.start()
        _gui_monitor = monitor
        return monitor
    except Exception as exc:
        print(f"[runtime] Warning: could not start GUI monitor: {exc}")
        return None
