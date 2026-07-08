import asyncio
import json
import os
import re
import select
import time
from typing import Literal, Optional

from fastapi import FastAPI, HTTPException, Query
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import RedirectResponse
from fastapi.staticfiles import StaticFiles
from pydantic import BaseModel

import multiagent
import runtime
import engine
import tools

app = FastAPI(title="ZapLauncher API", version="1.0.0")

# Agent ids currently running an autonomous LLM turn.
_busy_agents: set[str] = set()

# Agent ids under manual (human) control: the autonomous loop leaves them alone.
_manual_agents: set[str] = set()

# Dashboards are expected to run on a different origin during development.
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)


# ---------------------------------------------------------------------------
# Schemas
# ---------------------------------------------------------------------------

class PromptContent(BaseModel):
    name: str
    content: str


class TeamSummary(BaseModel):
    id: str
    shared_prompt: str
    model: str
    endpoint: str
    prompt_cache_key: str
    agent_ids: list[str]
    slots_available: Optional[int] = None
    slots_total: Optional[int] = None


class TeamUpdateRequest(BaseModel):
    shared_prompt: Optional[str] = None
    model: Optional[str] = None
    endpoint: Optional[str] = None
    prompt_cache_key: Optional[str] = None


class PersonalityCreateRequest(BaseModel):
    name: str
    content: str


class AgentDescription(BaseModel):
    id: str
    team_id: str
    personality_prompt: str
    cached_prompt: str
    facts_memory: list[str] = []


class AgentCreateRequest(BaseModel):
    team_id: str
    id: Optional[str] = None
    personality: Optional[str] = None
    random: bool = False


class AgentUpdateRequest(BaseModel):
    cached_prompt: Optional[str] = None
    personality_prompt: Optional[str] = None


class PromptAgentRequest(BaseModel):
    message: str


class ActionRequest(BaseModel):
    tool: str
    args: str = ""


class ConversationEntry(BaseModel):
    kind: Literal["user", "tool_use", "tool_result", "report", "llm_response"]
    content: Optional[str] = None
    tool_name: Optional[str] = None
    tool_input: Optional[str] = None
    message_id: Optional[str] = None
    malformed: Optional[bool] = None
    truncated: Optional[bool] = None
    timestamp: Optional[float] = None


class FeedEntry(BaseModel):
    agent_id: str
    team_id: str
    kind: Literal["user", "tool_use", "tool_result", "report", "llm_response"]
    content: Optional[str] = None
    tool_name: Optional[str] = None
    timestamp: Optional[float] = None


class UsageInfo(BaseModel):
    message_id: str
    usage: dict


class StatusResponse(BaseModel):
    server_connected: bool
    agents: dict[str, str]
    busy_agents: list[str] = []
    manual_agents: list[str] = []


class ControlRequest(BaseModel):
    manual: bool


class ControlResponse(BaseModel):
    agent_id: str
    manual: bool


class PlaybookAgentSpec(BaseModel):
    id: str
    personality: str


class PlaybookStep(BaseModel):
    agent: str
    tool: str
    args: str = ""
    expect: Optional[str] = None


class Playbook(BaseModel):
    name: str
    team: str
    agents: list[PlaybookAgentSpec] = []
    steps: list[PlaybookStep]


class StepResult(BaseModel):
    index: int
    agent: str
    tool: str
    args: str = ""
    result: Optional[str] = None
    expect: Optional[str] = None
    passed: Optional[bool] = None
    error: Optional[str] = None
    skipped: bool = False


class PlaybookRunResult(BaseModel):
    name: str
    ok: bool
    steps: list[StepResult]


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _get_team(team_id: str) -> multiagent.Team:
    team = runtime.teams.get(team_id)
    if team is None:
        raise HTTPException(404, f"Unknown team: {team_id}")
    return team


def _get_agent(agent_id: str) -> multiagent.Agent:
    agent = runtime.agents.get(agent_id)
    if agent is None:
        raise HTTPException(404, f"Unknown agent: {agent_id}")
    return agent


def _claim_agent(agent_id: str) -> None:
    """Mark an agent busy for the duration of a turn/action; 409 if it already is.

    Prevents two entry points (autonomous loop, /prompt, /action) from
    driving the same server socket concurrently.
    """
    if agent_id in _busy_agents:
        raise HTTPException(409, f"Agent is busy: {agent_id}")
    _busy_agents.add(agent_id)


def _agent_description(agent: multiagent.Agent) -> AgentDescription:
    return AgentDescription(
        id=agent.id,
        team_id=agent.team_id,
        personality_prompt=agent.personality_prompt,
        cached_prompt=agent.cached_prompt,
        facts_memory=list(agent.facts_memory),
    )


def _team_summary(team: multiagent.Team) -> TeamSummary:
    agent_ids = [
        agent.id for agent in runtime.agents.values()
        if agent.team_id == team["id"]
    ]
    live_count = sum(
        1 for aid in agent_ids
        if runtime.agents[aid].connection and runtime.agents[aid].connection.is_alive()
    )
    slots_total: Optional[int] = team.get("initial_slots")  # type: ignore[arg-type]
    slots_available: Optional[int] = (slots_total - live_count) if slots_total is not None else None

    return TeamSummary(
        id=team["id"],
        shared_prompt=team["shared_prompt"],
        model=team["model"],
        endpoint=team["endpoint"],
        prompt_cache_key=team["prompt_cache_key"],
        agent_ids=agent_ids,
        slots_available=slots_available,
        slots_total=slots_total,
    )


# ---------------------------------------------------------------------------
# Status
# ---------------------------------------------------------------------------

@app.on_event("startup")
async def _start_autonomous_loop():
    asyncio.create_task(_autonomous_loop())


async def _autonomous_loop():
    """Event-driven turn dispatcher.

    Instead of a fixed 500 ms poll (which added up to half a second of dead
    time to every single turn), block in select() on all idle agent sockets
    and wake the moment the server sends anything. A 'waiting N' notification
    then triggers an LLM turn immediately. The 0.5 s select timeout only
    bounds how fast newly created agents join the watch set.
    """
    while True:
        try:
            dispatched = False
            watch: dict = {}
            for agent_id, agent in list(runtime.agents.items()):
                if agent_id in _busy_agents or agent_id in _manual_agents:
                    continue
                conn = agent.connection
                if not conn:
                    continue
                ms = conn.poll_waiting()
                if ms is not None:
                    _busy_agents.add(agent_id)
                    asyncio.create_task(_handle_waiting(agent_id, ms))
                    dispatched = True
                else:
                    watch[conn._sock] = agent_id
            if dispatched:
                # Let the dispatched turns start, then re-scan right away.
                await asyncio.sleep(0)
                continue
            if not watch:
                await asyncio.sleep(0.2)
                continue
            # Sleep until any watched socket has data (select does not consume it).
            await asyncio.to_thread(select.select, list(watch), [], [], 0.5)
        except Exception as exc:
            print(f"[autonomous loop] error: {exc}")
            await asyncio.sleep(0.5)


async def _handle_waiting(agent_id: str, timeout_ms: int):
    try:
        await asyncio.to_thread(_run_autonomous_turn, agent_id, timeout_ms)
    finally:
        _busy_agents.discard(agent_id)


def _run_autonomous_turn(agent_id: str, timeout_ms: int):
    """Blocking: consume the waiting notification, ask the LLM what to do, act."""
    agent = runtime.agents.get(agent_id)
    if not agent or not agent.connection:
        return
    # Consume the 'waiting N' line from the buffer / socket.
    try:
        line = agent.connection._recv_line()
        if not line.startswith("waiting "):
            print(f"[autonomous:{agent_id}] unexpected line while consuming notification: {line!r}")
            return
    except Exception as exc:
        print(f"[autonomous:{agent_id}] failed to consume notification: {exc}")
        return

    team = runtime.teams.get(agent.team_id)
    if not team:
        return

    try:
        engine.run_turn(team, agent, "Continue.", max_steps=8, autonomous=True)
    except Exception as exc:
        print(f"[autonomous:{agent_id}] LLM turn error: {exc}")



@app.get("/status", response_model=StatusResponse)
def get_status():
    agent_statuses: dict[str, str] = {}
    for agent_id, agent in runtime.agents.items():
        if agent.connection is None:
            agent_statuses[agent_id] = "disconnected"
        elif agent.connection.is_alive():
            agent_statuses[agent_id] = "connected"
        else:
            agent.connection = None  # eagerly release dead socket
            agent_statuses[agent_id] = "disconnected"
    return StatusResponse(
        server_connected=multiagent.server_connected,
        agents=agent_statuses,
        busy_agents=list(_busy_agents),
        manual_agents=list(_manual_agents),
    )


# ---------------------------------------------------------------------------
# Preset prompts
# ---------------------------------------------------------------------------

@app.get("/prompts/teams", response_model=list[PromptContent])
def get_team_prompts():
    return [
        PromptContent(name=team_id, content=multiagent.get_team_prompt_content(team_id))
        for team_id in multiagent.list_team_ids()
    ]


@app.get("/prompts/teams/{team_id}", response_model=PromptContent)
def get_team_prompt(team_id: str):
    try:
        content = multiagent.get_team_prompt_content(team_id)
    except FileNotFoundError:
        raise HTTPException(404, f"Unknown team prompt: {team_id}")
    return PromptContent(name=team_id, content=content)


@app.get("/prompts/personalities", response_model=list[PromptContent])
def get_personality_prompts():
    return [
        PromptContent(name=name, content=multiagent.load_personality_prompt(name))
        for name in multiagent.list_personality_names()
    ]


@app.get("/prompts/personalities/{name}", response_model=PromptContent)
def get_personality_prompt(name: str):
    try:
        content = multiagent.load_personality_prompt(name)
    except FileNotFoundError:
        raise HTTPException(404, f"Unknown personality: {name}")
    return PromptContent(name=name, content=content)


@app.get("/prompts/protocol", response_model=PromptContent)
def get_protocol_prompt():
    return PromptContent(name="protocol", content=multiagent.load_protocol_prompt())


@app.put("/prompts/protocol", response_model=PromptContent)
def update_protocol_prompt(req: PromptContent):
    multiagent.save_protocol_file(req.content)
    for agent in runtime.agents.values():
        team = runtime.teams.get(agent.team_id)
        if team:
            agent.cached_prompt = multiagent.build_cached_prompt(team)
    return PromptContent(name="protocol", content=req.content)


@app.get("/prompts/trantorian", response_model=PromptContent)
def get_trantorian_prompt():
    return PromptContent(name="trantorian", content=multiagent.load_trantorian_prompt())


# ---------------------------------------------------------------------------
# Teams
# ---------------------------------------------------------------------------

@app.get("/teams", response_model=list[TeamSummary])
def list_teams():
    return [_team_summary(team) for team in runtime.teams.values()]


@app.patch("/teams/{team_id}", response_model=TeamSummary)
def update_team(team_id: str, req: TeamUpdateRequest):
    team = _get_team(team_id)

    if req.shared_prompt is not None:
        team["shared_prompt"] = req.shared_prompt
        multiagent.save_team_file(team_id, req.shared_prompt)
        for agent in runtime.agents.values():
            if agent.team_id == team_id:
                agent.cached_prompt = multiagent.build_cached_prompt(team)
    if req.model is not None:
        team["model"] = req.model
    if req.endpoint is not None:
        team["endpoint"] = req.endpoint
    if req.prompt_cache_key is not None:
        team["prompt_cache_key"] = req.prompt_cache_key

    return _team_summary(team)


# ---------------------------------------------------------------------------
# Personalities
# ---------------------------------------------------------------------------

@app.post("/personalities", response_model=PromptContent, status_code=201)
def create_personality(req: PersonalityCreateRequest):
    if req.name in multiagent.list_personality_names():
        raise HTTPException(409, f"Personality already exists: {req.name}")

    multiagent.save_personality_file(req.name, req.content)
    return PromptContent(name=req.name, content=req.content)


@app.put("/prompts/personalities/{name}", response_model=PromptContent)
def update_personality_prompt(name: str, req: PromptContent):
    if name not in multiagent.list_personality_names():
        raise HTTPException(404, f"Unknown personality: {name}")
    multiagent.save_personality_file(name, req.content)
    return PromptContent(name=name, content=req.content)


# ---------------------------------------------------------------------------
# Agents
# ---------------------------------------------------------------------------

@app.get("/agents/{agent_id}", response_model=AgentDescription)
def get_agent(agent_id: str):
    return _agent_description(_get_agent(agent_id))


@app.post("/agents", response_model=AgentDescription, status_code=201)
def create_agent_route(req: AgentCreateRequest):
    team = _get_team(req.team_id)

    # The monitor must be listening BEFORE the handshake, or it misses the pnw.
    monitor = runtime.get_gui_monitor()
    try:
        if req.random or not req.personality:
            agent = multiagent.random_agent(team, id=req.id)
        else:
            if req.personality not in multiagent.list_personality_names():
                raise HTTPException(404, f"Unknown personality: {req.personality}")
            agent = multiagent.create_agent(req.id or req.personality, team, req.personality)
    except HTTPException:
        raise
    except ConnectionError as exc:
        raise HTTPException(503, str(exc))
    except Exception as exc:
        raise HTTPException(500, f"Failed to create agent: {exc}")

    if agent.id in runtime.agents:
        raise HTTPException(409, f"Agent already exists: {agent.id}")

    if monitor and agent.connection is not None:
        agent.player_id = monitor.claim_player(team["id"])

    runtime.agents[agent.id] = agent
    return _agent_description(agent)


@app.patch("/agents/{agent_id}", response_model=AgentDescription)
def update_agent(agent_id: str, req: AgentUpdateRequest):
    agent = _get_agent(agent_id)

    if req.cached_prompt is not None:
        agent.cached_prompt = req.cached_prompt
    if req.personality_prompt is not None:
        agent.personality_prompt = req.personality_prompt

    return _agent_description(agent)


@app.get("/agents/{agent_id}/conversation", response_model=list[ConversationEntry])
def get_conversation(agent_id: str, since: int = Query(default=0, ge=0)):
    return _get_agent(agent_id).history[since:]


@app.post("/agents/{agent_id}/prompt", response_model=list[ConversationEntry])
def prompt_agent_route(agent_id: str, req: PromptAgentRequest):
    agent = _get_agent(agent_id)
    team = _get_team(agent.team_id)
    _claim_agent(agent_id)
    try:
        return engine.run_turn(team, agent, req.message)
    finally:
        _busy_agents.discard(agent_id)


@app.post("/agents/{agent_id}/action", response_model=list[ConversationEntry])
def run_agent_action(agent_id: str, req: ActionRequest):
    agent = _get_agent(agent_id)
    _claim_agent(agent_id)
    try:
        result = tools.use_tool(req.tool, req.args, agent)
    except ValueError as exc:
        raise HTTPException(400, str(exc))
    finally:
        _busy_agents.discard(agent_id)
    # Deliver any broadcast messages that arrived while the command was running
    if agent.connection is not None:
        received = agent.connection.drain_messages()
        if received:
            result += "\n" + "\n".join(received)
    now = time.time()
    entries = [
        {"kind": "tool_use", "tool_name": req.tool, "tool_input": (req.tool + " " + req.args).strip(), "timestamp": now},
        {"kind": "tool_result", "tool_name": req.tool, "content": result, "timestamp": now},
    ]
    agent.history.extend(entries)
    return entries


# ---------------------------------------------------------------------------
# Live feed
# ---------------------------------------------------------------------------

@app.get("/feed", response_model=list[FeedEntry])
def get_feed(since: float = Query(default=0.0)):
    entries: list[FeedEntry] = []
    for agent_id, agent in runtime.agents.items():
        for entry in agent.history:
            ts = entry.get("timestamp", 0) or 0
            if ts > since:
                entries.append(FeedEntry(
                    agent_id=agent_id,
                    team_id=agent.team_id,
                    kind=entry["kind"],
                    content=entry.get("content"),
                    tool_name=entry.get("tool_name"),
                    timestamp=ts,
                ))
    entries.sort(key=lambda e: e.timestamp or 0)
    return entries


# ---------------------------------------------------------------------------
# Token usage
# ---------------------------------------------------------------------------

@app.get("/usage/{message_id}", response_model=UsageInfo)
def get_usage(message_id: str):
    usage = runtime.usage_log.get(message_id)
    if usage is None:
        raise HTTPException(404, f"Unknown message id: {message_id}")
    return UsageInfo(message_id=message_id, usage=usage)


# ---------------------------------------------------------------------------
# Manual control (player POV)
# ---------------------------------------------------------------------------

class AgentStateResponse(BaseModel):
    agent_id: str
    player_id: Optional[int] = None
    x: Optional[int] = None
    y: Optional[int] = None
    orientation: Optional[int] = None
    facing: Optional[str] = None
    level: Optional[int] = None
    inventory: Optional[dict] = None


@app.get("/agents/{agent_id}/state", response_model=AgentStateResponse)
def get_agent_state(agent_id: str):
    """Authoritative state mirrored from the server's GUI event stream:
    absolute position, facing (north/east/south/west), level, inventory.
    Fields are null when no GUI monitor is available for this agent."""
    agent = _get_agent(agent_id)
    monitor = runtime.get_gui_monitor()
    if not monitor or agent.player_id is None:
        return AgentStateResponse(agent_id=agent_id, player_id=agent.player_id)
    player = monitor.get_player(agent.player_id)
    if not player:
        return AgentStateResponse(agent_id=agent_id, player_id=agent.player_id)
    return AgentStateResponse(
        agent_id=agent_id,
        player_id=agent.player_id,
        x=player.get("x"),
        y=player.get("y"),
        orientation=player.get("orientation"),
        facing=multiagent.FACING_NAMES.get(player.get("orientation")),
        level=player.get("level"),
        inventory=player.get("inventory"),
    )


@app.post("/agents/{agent_id}/control", response_model=ControlResponse)
def set_agent_control(agent_id: str, req: ControlRequest):
    """Give a human the wheel: while manual, the autonomous LLM loop skips
    this agent entirely; actions come in through /agents/{id}/action."""
    _get_agent(agent_id)
    if req.manual:
        _manual_agents.add(agent_id)
    else:
        _manual_agents.discard(agent_id)
    return ControlResponse(agent_id=agent_id, manual=req.manual)


# ---------------------------------------------------------------------------
# Playbooks (scene mode)
# ---------------------------------------------------------------------------

PLAYBOOKS_DIR = os.path.join(multiagent.PROMPT_DATABASE_DIR, "playbooks")

_PLAYBOOK_NAME_RE = re.compile(r"^[A-Za-z0-9_-]+$")


def _playbook_path(name: str) -> str:
    if not _PLAYBOOK_NAME_RE.match(name):
        raise HTTPException(422, "Playbook name must match [A-Za-z0-9_-]+")
    return os.path.join(PLAYBOOKS_DIR, f"{name}.json")


def _load_playbook(name: str) -> Playbook:
    path = _playbook_path(name)
    try:
        with open(path, "r", encoding="utf-8") as f:
            data = json.load(f)
    except FileNotFoundError:
        raise HTTPException(404, f"Unknown playbook: {name}")
    except json.JSONDecodeError as exc:
        raise HTTPException(500, f"Playbook file is not valid JSON: {exc}")
    data["name"] = name
    return Playbook(**data)


@app.get("/playbooks", response_model=list[Playbook])
def list_playbooks():
    os.makedirs(PLAYBOOKS_DIR, exist_ok=True)
    names = sorted(
        os.path.splitext(f)[0]
        for f in os.listdir(PLAYBOOKS_DIR)
        if f.endswith(".json")
    )
    return [_load_playbook(name) for name in names]


@app.get("/playbooks/{name}", response_model=Playbook)
def get_playbook(name: str):
    return _load_playbook(name)


@app.put("/playbooks/{name}", response_model=Playbook)
def save_playbook(name: str, playbook: Playbook):
    path = _playbook_path(name)
    playbook.name = name
    os.makedirs(PLAYBOOKS_DIR, exist_ok=True)
    with open(path, "w", encoding="utf-8") as f:
        json.dump(playbook.model_dump(), f, indent=2)
    return playbook


def _ensure_playbook_agents(playbook: Playbook) -> None:
    team = runtime.teams.get(playbook.team)
    if team is None:
        raise HTTPException(404, f"Unknown team: {playbook.team}")
    monitor = runtime.get_gui_monitor()
    for spec in playbook.agents:
        if spec.id in runtime.agents:
            continue
        if spec.personality not in multiagent.list_personality_names():
            raise HTTPException(404, f"Unknown personality: {spec.personality}")
        try:
            agent = multiagent.create_agent(spec.id, team, spec.personality)
        except ConnectionError as exc:
            raise HTTPException(503, f"Could not connect agent '{spec.id}': {exc}")
        if monitor and agent.connection is not None:
            agent.player_id = monitor.claim_player(team["id"])
        runtime.agents[agent.id] = agent


@app.post("/playbooks/{name}/run", response_model=PlaybookRunResult)
def run_playbook(name: str):
    """Execute a playbook scene: create/claim its agents, put them under
    manual control (the LLM loop must not interfere), then run every step
    in order. A step with `expect` passes when the result contains that
    substring; hard errors stop the run and mark remaining steps skipped."""
    playbook = _load_playbook(name)
    _ensure_playbook_agents(playbook)

    scene_agents = {spec.id for spec in playbook.agents} | {s.agent for s in playbook.steps}
    for agent_id in scene_agents:
        if agent_id in _busy_agents:
            raise HTTPException(409, f"Agent is busy: {agent_id}")
    _manual_agents.update(scene_agents)

    results: list[StepResult] = []
    ok = True
    aborted = False
    try:
        for index, step in enumerate(playbook.steps):
            if aborted:
                results.append(StepResult(
                    index=index, agent=step.agent, tool=step.tool, args=step.args,
                    expect=step.expect, skipped=True,
                ))
                continue

            agent = runtime.agents.get(step.agent)
            if agent is None:
                results.append(StepResult(
                    index=index, agent=step.agent, tool=step.tool, args=step.args,
                    expect=step.expect, error=f"Unknown agent: {step.agent}", passed=False,
                ))
                ok = False
                aborted = True
                continue

            try:
                result = tools.use_tool(step.tool, step.args, agent)
            except ValueError as exc:
                results.append(StepResult(
                    index=index, agent=step.agent, tool=step.tool, args=step.args,
                    expect=step.expect, error=str(exc), passed=False,
                ))
                ok = False
                aborted = True
                continue

            now = time.time()
            agent.history.extend([
                {"kind": "tool_use", "tool_name": step.tool,
                 "tool_input": (step.tool + " " + step.args).strip(), "timestamp": now},
                {"kind": "tool_result", "tool_name": step.tool, "content": result, "timestamp": now},
            ])

            passed: Optional[bool] = None
            if step.expect is not None:
                passed = step.expect in result
                if not passed:
                    ok = False
            results.append(StepResult(
                index=index, agent=step.agent, tool=step.tool, args=step.args,
                result=result, expect=step.expect, passed=passed,
            ))
    finally:
        _manual_agents.difference_update(scene_agents)

    return PlaybookRunResult(name=name, ok=ok, steps=results)


# ---------------------------------------------------------------------------
# Point-of-view frontends
# ---------------------------------------------------------------------------

_BASE_DIR = os.path.dirname(os.path.abspath(__file__))


@app.get("/", include_in_schema=False)
def root():
    return RedirectResponse(url="/leader/")


app.mount("/leader", StaticFiles(directory=os.path.join(_BASE_DIR, "dashboard"), html=True), name="leader")
app.mount("/player", StaticFiles(directory=os.path.join(_BASE_DIR, "player"), html=True), name="player")
app.mount("/playbook", StaticFiles(directory=os.path.join(_BASE_DIR, "playbook"), html=True), name="playbook")
