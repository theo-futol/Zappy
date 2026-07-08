"""Playbook CRUD + scene-runner tests (functional and end-to-end vs the fake server)."""

from fastapi.testclient import TestClient

import api
import runtime

client = TestClient(api.app)


VALID = {
    "name": "scene1",
    "team": "REDS",
    "agents": [{"id": "alpha", "personality": "guard"}],
    "steps": [
        {"agent": "alpha", "tool": "look", "args": "", "expect": "tile0"},
        {"agent": "alpha", "tool": "forward", "args": "", "expect": "ok"},
    ],
}


def test_playbook_save_get_list_roundtrip(clean_runtime):
    res = client.put("/playbooks/scene1", json=VALID)
    assert res.status_code == 200

    got = client.get("/playbooks/scene1").json()
    assert got["team"] == "REDS"
    assert len(got["steps"]) == 2

    names = [p["name"] for p in client.get("/playbooks").json()]
    assert names == ["scene1"]


def test_playbook_invalid_name_rejected(clean_runtime):
    res = client.put("/playbooks/../evil", json=VALID)
    assert res.status_code in (404, 422)
    res = client.put("/playbooks/has%20space", json=VALID)
    assert res.status_code == 422


def test_playbook_unknown_returns_404(clean_runtime):
    assert client.get("/playbooks/nope").status_code == 404
    assert client.post("/playbooks/nope/run").status_code == 404


def test_run_creates_agents_and_checks_expectations(clean_runtime, server_env):
    client.put("/playbooks/scene1", json=VALID)
    res = client.post("/playbooks/scene1/run")
    assert res.status_code == 200
    run = res.json()
    assert run["ok"] is True
    assert [s["passed"] for s in run["steps"]] == [True, True]
    # the scene created and connected its agent
    assert "alpha" in runtime.agents
    assert runtime.agents["alpha"].connection is not None
    # steps were recorded in the agent's history (visible in the leader view)
    kinds = [e["kind"] for e in runtime.agents["alpha"].history]
    assert kinds == ["tool_use", "tool_result"] * 2
    # manual control released after the run
    assert "alpha" not in api._manual_agents


def test_run_failed_expectation_marks_scene_failed(clean_runtime, server_env, fake_server):
    fake_server.responses["Forward"] = "ko"
    client.put("/playbooks/scene1", json=VALID)
    run = client.post("/playbooks/scene1/run").json()
    assert run["ok"] is False
    assert run["steps"][1]["passed"] is False
    # a failed expectation does NOT abort the run
    assert all(not s["skipped"] for s in run["steps"])


def test_run_hard_error_skips_remaining_steps(clean_runtime, server_env):
    playbook = {
        "name": "broken",
        "team": "REDS",
        "agents": [{"id": "alpha", "personality": "guard"}],
        "steps": [
            {"agent": "alpha", "tool": "teleport", "args": ""},
            {"agent": "alpha", "tool": "forward", "args": "", "expect": "ok"},
        ],
    }
    client.put("/playbooks/broken", json=playbook)
    run = client.post("/playbooks/broken/run").json()
    assert run["ok"] is False
    assert "Unknown tool" in run["steps"][0]["error"]
    assert run["steps"][1]["skipped"] is True


def test_run_unknown_team_404(clean_runtime):
    playbook = dict(VALID, team="NOT_A_TEAM", name="badteam")
    client.put("/playbooks/badteam", json=playbook)
    assert client.post("/playbooks/badteam/run").status_code == 404


def test_run_busy_agent_409(clean_runtime, server_env):
    client.put("/playbooks/scene1", json=VALID)
    client.post("/playbooks/scene1/run")  # creates alpha
    api._busy_agents.add("alpha")
    assert client.post("/playbooks/scene1/run").status_code == 409


def test_run_delivers_broadcasts_into_step_results(clean_runtime, server_env, fake_server):
    """Conversation scenes assert on heard broadcasts: 'message K, ...' lines
    arriving during a step must land in that step's result."""
    fake_server.responses["Inventory"] = [
        "message 3, Berta_the_merchant_here",
        "[food 10, linemate 0, deraumere 0, sibur 0, mendiane 0, phiras 0, thystame 0]",
    ]
    playbook = {
        "name": "hear",
        "team": "REDS",
        "agents": [{"id": "listener", "personality": "guard"}],
        "steps": [
            {"agent": "listener", "tool": "inventory", "args": "", "expect": "message"},
        ],
    }
    client.put("/playbooks/hear", json=playbook)
    run = client.post("/playbooks/hear/run").json()
    assert run["ok"] is True, run
    assert "message 3, Berta the merchant here" in run["steps"][0]["result"]


def test_step_wait_ms_delays_execution(clean_runtime, server_env):
    playbook = {
        "name": "paced",
        "team": "REDS",
        "agents": [{"id": "slowpoke", "personality": "guard"}],
        "steps": [
            {"agent": "slowpoke", "tool": "forward", "args": "", "expect": "ok", "wait_ms": 400},
        ],
    }
    client.put("/playbooks/paced", json=playbook)
    import time as _time
    start = _time.monotonic()
    run = client.post("/playbooks/paced/run").json()
    assert run["ok"] is True
    assert _time.monotonic() - start >= 0.4


def test_e2e_scene_drives_fake_server(clean_runtime, server_env, fake_server):
    """End-to-end: playbook -> agent creation -> TCP protocol -> assertions."""
    scene = {
        "name": "e2e",
        "team": "REDS",
        "agents": [{"id": "runner", "personality": "explorator"}],
        "steps": [
            {"agent": "runner", "tool": "look", "args": "", "expect": "tile0"},
            {"agent": "runner", "tool": "take", "args": "food", "expect": "ok"},
            {"agent": "runner", "tool": "broadcast", "args": "scene running now", "expect": "ok"},
            {"agent": "runner", "tool": "incantation", "args": "", "expect": "Elevation underway"},
        ],
    }
    client.put("/playbooks/e2e", json=scene)
    run = client.post("/playbooks/e2e/run").json()
    assert run["ok"] is True, run
    ai_commands = [cmd for team, cmd in fake_server.received if team == "REDS"]
    assert "Look" in ai_commands
    assert "Take food" in ai_commands
    assert "Broadcast scene_running_now" in ai_commands
    assert "Incantation" in ai_commands
