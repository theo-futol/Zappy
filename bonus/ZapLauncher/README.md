# ZapLauncher

LLM agents that roleplay a personality inside a shared "team" world, can call
a small set of tools, and can be driven from a CLI, a REST API, or a web
dashboard.

## TL;DR (for everyone)

- You define **teams** (a shared fictional world, e.g. a medieval village or
  a cyberpunk city) and **personalities** (e.g. "a greedy merchant").
- An **agent** = one team + one personality. Talk to it, and it answers in
  character — either with a plain reply, or by using a tool (like "look
  around") and reacting to the result.
- Everything is run with Docker. Once started, you get three points of view
  on the same game, all served by the API itself:
  - `/leader` — the **team-leader dashboard**: create teams/agents, chat with
    them, watch the live feed,
  - `/player` — the **player POV**: take manual control of one Trantorian and
    play it yourself (keyboard or buttons),
  - `/playbook` — **playbook scenes**: run a whole team through a predefined
    sequence of actions, with per-step assertions (great for testing),
  - plus the **REST API** (with auto-generated docs at `/docs`) behind all three.

## TL;DR (for developers)

- `prompt_database/` is the source of truth for all prompt text (teams,
  personalities, protocol, lore, playbooks). Nothing is hardcoded in Python.
- `multiagent.py` builds an `Agent`'s full prompt as two parts: a **cached
  prefix** (shared across a team, cheap to reuse) and a **personality
  suffix** (agent-specific). It also owns the TCP connections to the game
  server (`ServerConnection` / `GraphicConnection`).
- `engine.py` drives one conversational turn: call the LLM, parse its
  `USE`/`REPORT` answer, run the tools it asked for, loop until a `REPORT`
  (capped at 8 LLM calls per turn).
- `runtime.py` is the single in-memory source of truth ("the current loop")
  shared by every API entry point.
- `api.py` exposes that runtime over HTTP, runs the autonomous turn
  dispatcher, and serves the three frontends.
- `dashboard/`, `player/`, `playbook/` are static, build-free HTML/CSS/JS
  clients for the API (served at `/leader`, `/player`, `/playbook`).
- `tests/` is a pytest suite (functional + end-to-end against a fake Zappy
  server; see "Testing" below).

---

## How it works

### Teams, personalities, agents

- A **team** is a shared fictional world (lore, NPC names, rules) plus which
  LLM/model/endpoint to use for it.
- A **personality** is a short, reusable character description (e.g. "a
  loyal town guard").
- An **agent** is the combination of one team + one personality, with its
  own conversation history. Multiple agents can share the same team or the
  same personality.

### Prompt building (the part that matters)

Every agent's system prompt is assembled from **4 layers, in this order**:

1. **Protocol** — `prompt_database/protocol.txt`: tells the LLM it must
   answer with either `USE <tool_name>` or `REPORT <text>` (see below).
2. **Trantorian lore** — `prompt_database/TRANTORIAN.md`: a baseline lore
   layer applied to every agent regardless of team.
3. **Team shared prompt** — `prompt_database/teams/<team_id>.txt`: the
   world that team's agents believe they live in.
4. **Personality prompt** — `prompt_database/personalities/<name>.txt`: what
   makes this particular agent unique.

Layers 1–3 are concatenated once into `Agent.cached_prompt` (the same for
every agent of a team — a stable prefix), since LLM providers can cache a
repeated prompt prefix and bill/serve it cheaper. Layer 4 is kept separate as
`Agent.personality_prompt` because it's agent-specific and would break that
shared prefix if merged in.

At request time (`multiagent.build_prompt`), the two are sent as two
`system` messages followed by the user's message — `cached_prompt` always
comes first so the prefix stays identical across calls for the same team.

### Where predefined prompts live

```
prompt_database/
├── protocol.txt           # the USE/REPORT contract given to every agent
├── TRANTORIAN.md          # shared lore layer, given to every agent
├── teams/
│   ├── villagers.txt      # one .txt per team, filename = team id
│   └── cyberpunk.txt
└── personalities/
    ├── merchant.txt       # one .txt per personality, filename = its name
    ├── guard.txt
    ├── loyalist.txt
    └── rebel.txt
```

These files are **plain text, hand-editable**, and also what the API
reads/writes when you create a new team or personality — there is no
database engine, the filesystem *is* the database.

### The USE / REPORT protocol and tools

Agents answer with one or more lines of:
- `USE <tool_name> [args]` — run one of the tools in `tools.py` (`forward`,
  `left`, `right`, `look`, `inventory`, `broadcast`, `connect_nbr`, `fork`,
  `eject`, `take`, `set`, `incantation`, plus the local memory tools
  `remember`/`forget`) against the agent's live server connection, feed the
  result back, and let it respond again. Several USE lines in one response
  are executed in order (fewer LLM calls = faster game).

Agents also have a **persistent personal memory**: `USE remember <fact>`
stores up to 30 facts (relationships, deals, ritual crews) that are injected
into the system context on every turn — surviving far beyond the
conversation window — and `USE forget <number>` drops outdated ones. The
memory is visible in the `/leader` agent view and exposed as
`facts_memory` on `GET /agents/{id}`.
- `REPORT <text>` — end the turn; `<text>` is the agent's final answer.

`engine.run_turn` loops this (capped at 8 LLM calls) and logs every step as a
structured entry (`user` / `llm_response` / `tool_use` / `tool_result` /
`report`) in `agent.history` — this is what lets the API/dashboard show
"tools used" without guessing from raw text, and is also why the cached
system prompt never leaks into a conversation view: it's never part of the
history.

### Token usage & caching

Every LLM call is logged under a random `message_id` with its token usage
(prompt/completion/total, including cached-token counts the provider
returns). `report` and `tool_use` history entries carry that `message_id` so
usage can be looked up per message.

---

## Infrastructure / where things are accessible

Everything runs in **one Docker container** built from the `Dockerfile`,
normally via the compose file one directory up (`docker compose up
zaplauncher`). This requires a `.env` file next to `compose.yml` with:

```
API_KEY=<your Mistral API key>
```

Once running:

| What | Where |
|---|---|
| REST API | `http://localhost:8000` |
| Interactive API docs (Swagger UI) | `http://localhost:8000/docs` |
| Alternative API docs (ReDoc) | `http://localhost:8000/redoc` |
| Raw OpenAPI schema | `http://localhost:8000/openapi.json` |
| Team-leader dashboard | `http://localhost:8000/leader/` |
| Player POV (manual control) | `http://localhost:8000/player/` |
| Playbook scenes | `http://localhost:8000/playbook/` |

`/` redirects to `/leader/`. The frontends are plain static files served by
the API itself, so there is no build step and no CORS configuration needed;
they can still be opened straight from disk (they fall back to
`http://localhost:8000` as API base in that case).

The container's **default command runs the API** (`uvicorn api:app --host
0.0.0.0 --port 8000`).

## Testing

```bash
# Functional + e2e tests (fake Zappy server, scripted LLM — no network, no key)
docker run --rm -v $PWD:/app -w /app -e API_KEY=test python:trixie \
    bash -c "pip install -q -r requirements-dev.txt && pytest"

# End-to-end against the real C++ server (from the bonus/ directory).
# ZAPPY_CLIENTS=20: every connection consumes an egg and the suite opens ~8.
docker compose build server && ZAPPY_CLIENTS=20 docker compose up -d server
docker run --rm --network bonus_zappy-net -v $PWD/ZapLauncher:/app -w /app \
    -e API_KEY=test -e ZAPPY_E2E_HOST=server -e ZAPPY_E2E_PORT=4242 \
    python:trixie bash -c "pip install -q -r requirements-dev.txt && pytest tests/test_e2e_real_server.py"
```

LLM calls go out to Mistral's API (`https://api.mistral.ai/v1`, OpenAI-compatible
client) using the model/endpoint/cache key configured per team.

### Code map

| File | Role |
|---|---|
| `multiagent.py` | Core types (`Team`, `Agent`, `HistoryEntry`), server connections, all `prompt_database` read/write helpers + prompt assembly |
| `tools.py` | The tools agents can `USE` |
| `runtime.py` | In-memory state shared by every entry point: running `teams`, `agents`, `usage_log` |
| `engine.py` | LLM call + USE/REPORT turn loop |
| `api.py` | FastAPI app: every route, the autonomous turn dispatcher, playbook runner, static frontends |
| `dashboard/` | Team-leader frontend, served at `/leader` (no build step, no dependencies) |
| `player/` | Player-POV frontend (manual control), served at `/player` |
| `playbook/` | Playbook-scene frontend, served at `/playbook` |
| `prompt_database/` | All prompt text + playbooks, organized as described above |
| `tests/` | pytest suite (fake Zappy server, scripted LLM, API tests, e2e) |

### API surface (see `/docs` for full schemas)

- `GET /prompts/teams[/​{id}]`, `/prompts/personalities[/​{name}]`,
  `/prompts/protocol`, `/prompts/trantorian` — browse preset prompts
- `GET /teams` · `PATCH /teams/{id}` — list/configure teams
- `POST /personalities` — create a personality
- `GET /agents/{id}` · `POST /agents` · `PATCH /agents/{id}` — describe,
  create, or live-edit a running agent's prompts
- `GET /agents/{id}/conversation` — structured turn-by-turn history
- `POST /agents/{id}/prompt` — send a message, get back the new history
  entries
- `POST /agents/{id}/action` — run one tool directly (no LLM); 409 if the
  agent is mid-turn
- `POST /agents/{id}/control` — take/release manual control of an agent
  (while manual, the autonomous loop leaves it alone)
- `GET /agents/{id}/state` — authoritative state mirrored from the server's
  GUI event stream (player id, position, **facing** north/east/south/west,
  level, inventory); powers the player POV's compass and isometric view
- `GET /playbooks` · `GET/PUT /playbooks/{name}` · `POST /playbooks/{name}/run`
  — list, read, save, and execute playbook scenes
- `GET /feed` — cross-agent activity feed
- `GET /status` — server link, per-agent connection state, busy + manual sets
- `GET /usage/{message_id}` — token usage for one LLM call
