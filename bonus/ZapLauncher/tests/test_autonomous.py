"""Tests for the event-driven autonomous turn dispatcher."""

import asyncio
import time

import api
import engine
import multiagent
import runtime


def connect_agent(fake_server, agent_id="auto-agent") -> multiagent.Agent:
    conn = multiagent.ServerConnection("127.0.0.1", fake_server.port, "REDS")
    agent = multiagent.Agent(
        id=agent_id, team_id="REDS", cached_prompt="c", personality_prompt="p", connection=conn,
    )
    runtime.agents[agent.id] = agent
    return agent


def run_loop_briefly(duration=0.8):
    async def _run():
        task = asyncio.create_task(api._autonomous_loop())
        await asyncio.sleep(duration)
        task.cancel()
        try:
            await task
        except asyncio.CancelledError:
            pass
    asyncio.run(_run())


def consuming_stub(record):
    """Like the real turn runner, consume the waiting line off the socket."""
    def stub(aid, ms):
        record.append((aid, ms))
        runtime.agents[aid].connection._recv_line()
    return stub


def test_waiting_notification_dispatches_turn(clean_runtime, fake_server, monkeypatch):
    agent = connect_agent(fake_server)
    dispatched = []
    monkeypatch.setattr(api, "_run_autonomous_turn", consuming_stub(dispatched))

    fake_server.push_to_ai("waiting 1200")
    run_loop_briefly()

    assert dispatched == [(agent.id, 1200)]
    assert agent.id not in api._busy_agents  # released after the turn


def test_manual_agents_are_skipped(clean_runtime, fake_server, monkeypatch):
    agent = connect_agent(fake_server)
    api._manual_agents.add(agent.id)
    dispatched = []
    monkeypatch.setattr(api, "_run_autonomous_turn", lambda aid, ms: dispatched.append(aid))

    fake_server.push_to_ai("waiting 1200")
    run_loop_briefly()

    assert dispatched == []


def test_busy_agents_are_not_double_dispatched(clean_runtime, fake_server, monkeypatch):
    agent = connect_agent(fake_server)
    api._busy_agents.add(agent.id)
    dispatched = []
    monkeypatch.setattr(api, "_run_autonomous_turn", lambda aid, ms: dispatched.append(aid))

    fake_server.push_to_ai("waiting 1200")
    run_loop_briefly()

    assert dispatched == []


def test_dispatch_is_prompt_not_polled(clean_runtime, fake_server, monkeypatch):
    """The event-driven loop must react well below the old 500 ms poll period."""
    agent = connect_agent(fake_server)
    dispatched_at = []

    def stub(aid, ms):
        dispatched_at.append(time.monotonic())
        runtime.agents[aid].connection._recv_line()
    monkeypatch.setattr(api, "_run_autonomous_turn", stub)

    async def _run():
        task = asyncio.create_task(api._autonomous_loop())
        await asyncio.sleep(0.3)  # loop is now blocked in select()
        start = time.monotonic()
        fake_server.push_to_ai("waiting 1000")
        await asyncio.sleep(0.4)
        task.cancel()
        try:
            await task
        except asyncio.CancelledError:
            pass
        return start

    start = asyncio.run(_run())
    assert dispatched_at, "turn was never dispatched"
    assert dispatched_at[0] - start < 0.35


def test_run_autonomous_turn_consumes_notification_and_runs_engine(
    clean_runtime, fake_server, monkeypatch
):
    agent = connect_agent(fake_server)
    fake_server.push_to_ai("waiting 900")
    deadline = time.time() + 2
    while agent.connection.poll_waiting() is None and time.time() < deadline:
        time.sleep(0.01)

    calls = []
    monkeypatch.setattr(engine, "run_turn", lambda team, ag, msg, **kw: calls.append((ag.id, msg, kw)))
    api._run_autonomous_turn(agent.id, 900)

    assert calls and calls[0][0] == agent.id
    assert calls[0][2].get("autonomous") is True
    # the waiting line was consumed off the socket
    assert agent.connection.poll_waiting() is None
