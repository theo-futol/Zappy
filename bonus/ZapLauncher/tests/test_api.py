"""API tests via FastAPI TestClient (no uvicorn, no LLM)."""

from fastapi.testclient import TestClient

import api
import multiagent
import runtime

client = TestClient(api.app)


def make_local_agent(agent_id="manual-tester") -> multiagent.Agent:
    agent = multiagent.Agent(
        id=agent_id, team_id="REDS", cached_prompt="c", personality_prompt="p", connection=None,
    )
    runtime.agents[agent.id] = agent
    return agent


def test_status_reports_manual_and_busy_agents(clean_runtime):
    make_local_agent()
    api._manual_agents.add("manual-tester")
    api._busy_agents.add("someone-else")
    data = client.get("/status").json()
    assert "manual-tester" in data["manual_agents"]
    assert "someone-else" in data["busy_agents"]
    assert data["agents"]["manual-tester"] == "disconnected"


def test_action_records_tool_use_and_result(clean_runtime):
    agent = make_local_agent()
    res = client.post(f"/agents/{agent.id}/action", json={"tool": "forward", "args": ""})
    assert res.status_code == 200
    entries = res.json()
    assert [e["kind"] for e in entries] == ["tool_use", "tool_result"]
    assert entries[0]["tool_input"] == "forward"
    assert entries[1]["content"] == "ok (no server)"
    assert len(agent.history) == 2


def test_action_on_busy_agent_returns_409(clean_runtime):
    agent = make_local_agent()
    api._busy_agents.add(agent.id)
    res = client.post(f"/agents/{agent.id}/action", json={"tool": "forward", "args": ""})
    assert res.status_code == 409


def test_action_unknown_tool_returns_400_and_releases_busy(clean_runtime):
    agent = make_local_agent()
    res = client.post(f"/agents/{agent.id}/action", json={"tool": "teleport", "args": ""})
    assert res.status_code == 400
    assert agent.id not in api._busy_agents  # must not stay claimed


def test_control_toggle(clean_runtime):
    agent = make_local_agent()
    res = client.post(f"/agents/{agent.id}/control", json={"manual": True})
    assert res.status_code == 200
    assert res.json() == {"agent_id": agent.id, "manual": True}
    assert agent.id in api._manual_agents

    res = client.post(f"/agents/{agent.id}/control", json={"manual": False})
    assert agent.id not in api._manual_agents


def test_control_unknown_agent_404(clean_runtime):
    res = client.post("/agents/ghost/control", json={"manual": True})
    assert res.status_code == 404


def test_agent_creation_connects_to_server(clean_runtime, server_env):
    res = client.post("/agents", json={"team_id": "REDS", "id": "sock-agent", "personality": "guard"})
    assert res.status_code == 201
    agent = runtime.agents["sock-agent"]
    assert agent.connection is not None
    assert agent.connection.map_size == "10 10"


def test_agent_creation_rejected_when_no_slots(clean_runtime, server_env, fake_server):
    fake_server.slots = 0
    res = client.post("/agents", json={"team_id": "REDS", "id": "late", "personality": "guard"})
    assert res.status_code == 503


def test_action_delivers_pending_broadcasts(clean_runtime, server_env, fake_server):
    client.post("/agents", json={"team_id": "REDS", "id": "listener", "personality": "guard"})
    fake_server.responses["Forward"] = ["message 5, enemy_spotted", "ok"]
    res = client.post("/agents/listener/action", json={"tool": "forward", "args": ""})
    result = res.json()[1]["content"]
    assert "ok" in result
    assert "message 5, enemy spotted" in result


def test_conversation_since_pagination(clean_runtime):
    agent = make_local_agent()
    client.post(f"/agents/{agent.id}/action", json={"tool": "forward", "args": ""})
    client.post(f"/agents/{agent.id}/action", json={"tool": "left", "args": ""})
    full = client.get(f"/agents/{agent.id}/conversation").json()
    assert len(full) == 4
    tail = client.get(f"/agents/{agent.id}/conversation?since=2").json()
    assert len(tail) == 2
    assert tail[0]["tool_name"] == "left"


def test_pov_frontends_are_served(clean_runtime):
    for path, marker in [
        ("/leader/", "ZapLauncher"),
        ("/player/", "Player POV"),
        ("/playbook/", "Playbook"),
    ]:
        res = client.get(path)
        assert res.status_code == 200, path
        assert marker.lower() in res.text.lower(), path


def test_root_redirects_to_leader(clean_runtime):
    res = client.get("/", follow_redirects=False)
    assert res.status_code in (302, 307)
    assert res.headers["location"] == "/leader/"


def test_player_pov_sprites_are_served(clean_runtime):
    """The player POV reuses the map GUI's sprites; they must be served."""
    for asset in ["grass", "golem", "food", "linemate", "deraumere",
                  "sibur", "mendiane", "phiras", "thystame"]:
        res = client.get(f"/player/assets/{asset}.png")
        assert res.status_code == 200, asset
        assert res.headers["content-type"] == "image/png", asset


def test_agent_description_exposes_facts_memory(clean_runtime):
    agent = make_local_agent()
    agent.facts_memory = ["Zaphod (REDS) is my partner"]
    data = client.get(f"/agents/{agent.id}").json()
    assert data["facts_memory"] == ["Zaphod (REDS) is my partner"]


def test_memory_editable_via_action_route(clean_runtime):
    agent = make_local_agent()
    res = client.post(f"/agents/{agent.id}/action",
                      json={"tool": "remember", "args": "met Trillian at the ritual"})
    assert res.status_code == 200
    assert agent.facts_memory == ["met Trillian at the ritual"]
    client.post(f"/agents/{agent.id}/action", json={"tool": "forget", "args": "1"})
    assert agent.facts_memory == []
