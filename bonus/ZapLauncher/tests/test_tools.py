"""Functional tests for the agent tools layer."""

import pytest

import tools
from multiagent import Agent, ServerConnection


def _agent(conn=None) -> Agent:
    return Agent(id="t", team_id="REDS", cached_prompt="", personality_prompt="", connection=conn)


def test_clean_look_strips_zero_counts_and_labels_tiles():
    raw = "[player food:2 linemate:0 , linemate:1 food:0 , , food:1 ]"
    cleaned = tools._clean_look(raw)
    assert "tile0: player food:2" in cleaned
    assert "tile1: linemate:1" in cleaned
    assert "tile2: (empty)" in cleaned
    assert "tile3: food:1" in cleaned
    assert "linemate:0" not in cleaned
    assert "food:0" not in cleaned


def test_broadcast_requires_message():
    result = tools.broadcast(_agent(), "")
    assert result.startswith("error")


def test_broadcast_encodes_spaces_as_underscores(fake_server, server_env):
    conn = ServerConnection("127.0.0.1", fake_server.port, "REDS")
    agent = _agent(conn)
    assert tools.broadcast(agent, "hello brave world") == "ok"
    assert ("REDS", "Broadcast hello_brave_world") in fake_server.received
    conn.close()


def test_take_requires_object():
    assert tools.take(_agent(), "").startswith("ko")


def test_take_ko_is_explained(fake_server, server_env):
    fake_server.responses["Take"] = "ko"
    conn = ServerConnection("127.0.0.1", fake_server.port, "REDS")
    result = tools.take(_agent(conn), "food")
    assert "different tile" in result  # the LLM-friendly explanation
    conn.close()


def test_unknown_tool_raises():
    with pytest.raises(ValueError):
        tools.use_tool("teleport", "", _agent())


def test_dead_connection_reports_death_and_clears_connection(fake_server):
    fake_server.responses["Forward"] = "dead"
    conn = ServerConnection("127.0.0.1", fake_server.port, "REDS")
    agent = _agent(conn)
    result = tools.use_tool("forward", "", agent)
    assert "[dead]" in result
    assert agent.connection is None


def test_tools_without_server_return_placeholders():
    agent = _agent()
    assert tools.use_tool("forward", "", agent) == "ok (no server)"
    assert "food" in tools.use_tool("inventory", "", agent)


def test_remember_stores_facts():
    agent = _agent()
    result = tools.remember(agent, "Zaphod (REDS) is my gathering partner")
    assert "remembered (1/" in result
    assert agent.facts_memory == ["Zaphod (REDS) is my gathering partner"]


def test_remember_requires_a_fact():
    agent = _agent()
    assert tools.remember(agent, "  ").startswith("error")
    assert agent.facts_memory == []


def test_remember_caps_memory_dropping_oldest():
    agent = _agent()
    for i in range(tools.MAX_FACTS + 2):
        tools.remember(agent, f"fact {i}")
    assert len(agent.facts_memory) == tools.MAX_FACTS
    assert agent.facts_memory[0] == "fact 2"  # two oldest dropped
    assert agent.facts_memory[-1] == f"fact {tools.MAX_FACTS + 1}"


def test_forget_removes_by_number():
    agent = _agent()
    tools.remember(agent, "keep me")
    tools.remember(agent, "drop me")
    result = tools.forget(agent, "2")
    assert "forgot: drop me" in result
    assert agent.facts_memory == ["keep me"]


def test_forget_rejects_bad_numbers():
    agent = _agent()
    tools.remember(agent, "only fact")
    assert tools.forget(agent, "").startswith("error")
    assert tools.forget(agent, "0").startswith("error")
    assert tools.forget(agent, "2").startswith("error")
    assert tools.forget(agent, "nope").startswith("error")
    assert agent.facts_memory == ["only fact"]


def test_memory_tools_work_without_server():
    agent = _agent()
    assert "remembered" in tools.use_tool("remember", "a social fact", agent)
    assert "forgot" in tools.use_tool("forget", "1", agent)
