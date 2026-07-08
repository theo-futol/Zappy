"""Tests for the GuiMonitor (player-id claiming + authoritative state mirror)
and the /agents/{id}/state route feeding the player POV page."""

import time

from fastapi.testclient import TestClient

import api
import multiagent
import runtime

client = TestClient(api.app)


def wait_until(predicate, timeout=2.0):
    deadline = time.time() + timeout
    while time.time() < deadline:
        if predicate():
            return True
        time.sleep(0.02)
    return False


def test_monitor_claims_player_id_on_agent_creation(clean_runtime, server_env):
    res = client.post("/agents", json={"team_id": "REDS", "id": "iso-hero", "personality": "guard"})
    assert res.status_code == 201
    agent = runtime.agents["iso-hero"]
    assert agent.player_id == 1


def test_state_route_reports_facing_level_inventory(clean_runtime, server_env):
    client.post("/agents", json={"team_id": "REDS", "id": "iso-hero", "personality": "guard"})
    state = client.get("/agents/iso-hero/state").json()
    assert state["player_id"] == 1
    assert state["x"] == 2 and state["y"] == 3
    assert state["orientation"] == 1
    assert state["facing"] == "north"
    assert state["level"] == 1
    assert state["inventory"]["food"] == 10


def test_state_updates_from_gui_events(clean_runtime, server_env, fake_server):
    client.post("/agents", json={"team_id": "REDS", "id": "iso-hero", "personality": "guard"})
    monitor = runtime.get_gui_monitor()

    fake_server.push_to_graphic("pipi 1 5 6 3 2 4 1 0 0 0 0 0")
    assert wait_until(lambda: (monitor.get_player(1) or {}).get("level") == 2)

    state = client.get("/agents/iso-hero/state").json()
    assert (state["x"], state["y"]) == (5, 6)
    assert state["facing"] == "south"
    assert state["level"] == 2
    assert state["inventory"] == {
        "food": 4, "linemate": 1, "deraumere": 0, "sibur": 0,
        "mendiane": 0, "phiras": 0, "thystame": 0,
    }

    fake_server.push_to_graphic("plv 1 3")
    assert wait_until(lambda: (monitor.get_player(1) or {}).get("level") == 3)
    fake_server.push_to_graphic("ppo 1 7 8 2")
    assert wait_until(lambda: (monitor.get_player(1) or {}).get("orientation") == 2)
    state = client.get("/agents/iso-hero/state").json()
    assert state["facing"] == "east"
    assert state["level"] == 3


def test_player_removed_on_death_event(clean_runtime, server_env, fake_server):
    client.post("/agents", json={"team_id": "REDS", "id": "doomed", "personality": "guard"})
    monitor = runtime.get_gui_monitor()
    assert wait_until(lambda: monitor.get_player(1) is not None)
    fake_server.push_to_graphic("pdi 1")
    assert wait_until(lambda: monitor.get_player(1) is None)
    state = client.get("/agents/doomed/state").json()
    assert state["facing"] is None


def test_state_without_server_is_all_null(clean_runtime, no_server_env):
    agent = multiagent.Agent(
        id="offline", team_id="REDS", cached_prompt="c", personality_prompt="p",
    )
    runtime.agents[agent.id] = agent
    state = client.get("/agents/offline/state").json()
    assert state["player_id"] is None
    assert state["facing"] is None


def test_two_agents_get_distinct_player_ids(clean_runtime, server_env):
    client.post("/agents", json={"team_id": "REDS", "id": "first", "personality": "guard"})
    client.post("/agents", json={"team_id": "REDS", "id": "second", "personality": "rebel"})
    assert runtime.agents["first"].player_id == 1
    assert runtime.agents["second"].player_id == 2
