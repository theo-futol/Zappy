import asyncio
import time
from typing import Literal, Optional

from fastapi import FastAPI, HTTPException, Query
from fastapi.middleware.cors import CORSMiddleware
from pydantic import BaseModel

import multiagent
import runtime
import engine
import tools

app = FastAPI(title="ZapLauncher API", version="1.0.0")

# Agent ids currently running an autonomous LLM turn.
_busy_agents: set[str] = set()

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


def _agent_description(agent: multiagent.Agent) -> AgentDescription:
    return AgentDescription(
        id=agent.id,
        team_id=agent.team_id,
        personality_prompt=agent.personality_prompt,
        cached_prompt=agent.cached_prompt,
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
    """Poll each connected agent for a 'waiting N' notification every 500 ms.
    When found, spawn a task that consumes the notification and runs one LLM turn."""
    while True:
        await asyncio.sleep(0.5)
        try:
            for agent_id, agent in list(runtime.agents.items()):
                if agent_id in _busy_agents:
                    continue
                conn = agent.connection
                if not conn:
                    continue
                ms = conn.poll_waiting()
                if ms is not None:
                    _busy_agents.add(agent_id)
                    asyncio.create_task(_handle_waiting(agent_id, ms))
        except Exception as exc:
            print(f"[autonomous loop] error: {exc}")


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
    return engine.run_turn(team, agent, req.message)


@app.post("/agents/{agent_id}/action", response_model=list[ConversationEntry])
def run_agent_action(agent_id: str, req: ActionRequest):
    agent = _get_agent(agent_id)
    try:
        result = tools.use_tool(req.tool, req.args, agent)
    except ValueError as exc:
        raise HTTPException(400, str(exc))
    now = time.time()
    entries = [
        {"kind": "tool_use", "tool_name": req.tool, "timestamp": now},
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
