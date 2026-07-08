"""Engine turn-loop tests with a scripted fake LLM (no network, no real model)."""

from types import SimpleNamespace

import pytest

import engine
from multiagent import Agent, Team


def make_team() -> Team:
    return Team(id="REDS", shared_prompt="team prompt", model="fake-model",
                endpoint="http://fake", prompt_cache_key="REDS")


def make_agent() -> Agent:
    return Agent(id="tester", team_id="REDS", cached_prompt="cached",
                 personality_prompt="personality", connection=None)


class FakeLLM:
    def __init__(self, answers):
        self.answers = list(answers)
        self.calls = []

    def create(self, **kwargs):
        # snapshot: the engine mutates the messages list after the call
        self.calls.append({**kwargs, "messages": list(kwargs.get("messages", []))})
        content = self.answers.pop(0)
        return SimpleNamespace(
            usage=None,
            choices=[SimpleNamespace(message=SimpleNamespace(content=content))],
        )


@pytest.fixture
def fake_llm(monkeypatch):
    def install(answers):
        llm = FakeLLM(answers)
        monkeypatch.setattr(engine.client.chat.completions, "create", llm.create)
        return llm
    return install


def kinds(entries):
    return [e["kind"] for e in entries]


def test_report_ends_turn(fake_llm):
    fake_llm(["REPORT all good"])
    entries = engine.run_turn(make_team(), make_agent(), "status?")
    assert kinds(entries) == ["user", "llm_response", "report"]
    assert entries[-1]["content"] == "all good"


def test_multi_use_chain_executes_in_order_then_next_call(fake_llm):
    llm = fake_llm(["USE forward\nUSE forward\nUSE look", "REPORT moved twice"])
    agent = make_agent()
    entries = engine.run_turn(make_team(), agent, "go")
    tool_uses = [e for e in entries if e["kind"] == "tool_use"]
    assert [e["tool_name"] for e in tool_uses] == ["forward", "forward", "look"]
    # every tool_use has a matching result
    assert len([e for e in entries if e["kind"] == "tool_result"]) == 3
    assert entries[-1]["kind"] == "report"
    assert len(llm.calls) == 2


def test_tool_use_records_full_input(fake_llm):
    fake_llm(["USE take food", "REPORT done"])
    entries = engine.run_turn(make_team(), make_agent(), "eat")
    tool_use = next(e for e in entries if e["kind"] == "tool_use")
    assert tool_use["tool_name"] == "take"
    assert tool_use["tool_input"] == "take food"


def test_malformed_response_is_logged_and_ends_turn(fake_llm):
    fake_llm(["I don't know what to do"])
    entries = engine.run_turn(make_team(), make_agent(), "hello")
    assert entries[-1]["kind"] == "llm_response"
    assert entries[-1]["malformed"] is True


def test_report_stops_processing_following_commands(fake_llm):
    fake_llm(["REPORT stopping here\nUSE forward"])
    entries = engine.run_turn(make_team(), make_agent(), "go")
    assert all(e["kind"] != "tool_use" for e in entries)


def test_unknown_tool_feeds_error_back(fake_llm):
    fake_llm(["USE teleport home", "REPORT could not teleport"])
    entries = engine.run_turn(make_team(), make_agent(), "go")
    result = next(e for e in entries if e["kind"] == "tool_result")
    assert "Unknown tool" in result["content"]


def test_max_steps_caps_the_loop(fake_llm):
    llm = fake_llm(["USE forward"] * 10)
    engine.run_turn(make_team(), make_agent(), "go", max_steps=3)
    assert len(llm.calls) == 3


def test_history_window_limits_context(fake_llm):
    agent = make_agent()
    for i in range(100):
        agent.history.append({"kind": "user", "content": f"old message {i}", "timestamp": 0})
    llm = fake_llm(["REPORT ok"])
    engine.run_turn(make_team(), agent, "latest")
    messages = llm.calls[0]["messages"]
    history_msgs = [m for m in messages if m["content"].startswith("old message")]
    assert len(history_msgs) <= engine._HISTORY_WINDOW


def test_old_tool_results_are_truncated_in_context():
    long_result = "x" * 500
    history = []
    for i in range(engine._FULL_DETAIL_TAIL + 5):
        history.append({"kind": "tool_result", "tool_name": "look", "content": long_result})
    messages = engine._history_to_messages(history)
    # the oldest entries are truncated, the newest keep full detail
    assert "…[truncated]" in messages[0]["content"]
    assert len(messages[0]["content"]) < 300
    assert "…[truncated]" not in messages[-1]["content"]


def test_autonomous_trigger_is_system_role(fake_llm):
    llm = fake_llm(["REPORT ok"])
    engine.run_turn(make_team(), make_agent(), "Continue.", autonomous=True)
    trigger = llm.calls[0]["messages"][-1]
    assert trigger["role"] == "system"
    assert trigger["content"] == "Continue."


def test_commander_trigger_is_user_role(fake_llm):
    llm = fake_llm(["REPORT ok"])
    engine.run_turn(make_team(), make_agent(), "hello")
    trigger = llm.calls[0]["messages"][-1]
    assert trigger["role"] == "user"
    assert "[Commander]" in trigger["content"]


def test_facts_memory_is_injected_as_system_message(fake_llm):
    agent = make_agent()
    agent.facts_memory = ["Zaphod (REDS) is trusted", "Marvin (BLUES) lies"]
    llm = fake_llm(["REPORT ok"])
    engine.run_turn(make_team(), agent, "hello")
    memory_msgs = [
        m for m in llm.calls[0]["messages"]
        if m["role"] == "system" and "Your memory" in m["content"]
    ]
    assert len(memory_msgs) == 1
    assert "1. Zaphod (REDS) is trusted" in memory_msgs[0]["content"]
    assert "2. Marvin (BLUES) lies" in memory_msgs[0]["content"]


def test_no_memory_message_when_memory_empty(fake_llm):
    llm = fake_llm(["REPORT ok"])
    engine.run_turn(make_team(), make_agent(), "hello")
    assert not any(
        "Your memory" in m["content"]
        for m in llm.calls[0]["messages"] if m["role"] == "system"
    )


def test_remember_via_use_persists_and_reaches_next_call(fake_llm):
    agent = make_agent()
    llm = fake_llm([
        "USE remember Trillian (REDS) answered my ritual call",
        "REPORT noted",
    ])
    engine.run_turn(make_team(), agent, "you met Trillian")
    assert agent.facts_memory == ["Trillian (REDS) answered my ritual call"]
    # the next turn sees the remembered fact in its system context
    llm2 = fake_llm(["REPORT ok"])
    engine.run_turn(make_team(), agent, "who do you trust?")
    assert any(
        "Trillian (REDS) answered my ritual call" in m["content"]
        for m in llm2.calls[0]["messages"] if m["role"] == "system"
    )
