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
- Everything is run with Docker. Once started, you get:
  - a **web dashboard** to create teams/agents and chat with them,
  - a **REST API** (with auto-generated docs) behind it,
  - a CLI if you just want a quick terminal chat.

## TL;DR (for developers)

- `prompt_database/` is the source of truth for all prompt text (teams,
  personalities, protocol, lore). Nothing is hardcoded in Python.
- `multiagent.py` builds an `Agent`'s full prompt as two parts: a **cached
  prefix** (shared across a team, cheap to reuse) and a **personality
  suffix** (agent-specific).
- `engine.py` drives one conversational turn: call the LLM, parse its
  `USE`/`REPORT` answer, run a tool if asked, loop until a `REPORT`.
- `runtime.py` is the single in-memory source of truth ("the current loop")
  shared by `main.py` (CLI) and `api.py` (REST API).
- `api.py` exposes that runtime over HTTP for the dashboard.
- `dashboard/` is a static, build-free HTML/CSS/JS client for the API.

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

Agents must answer with exactly one of:
- `USE <tool_name>` — run one of the tools in `tools.py` (currently `look`,
  `broadcast`, both placeholders) against the agent, feed the result back
  to the agent, and let it respond again.
- `REPORT <text>` — end the turn; `<text>` is the agent's final answer.

`engine.run_turn` loops this (capped at 5 steps) and logs every step as a
structured entry (`user` / `tool_use` / `tool_result` / `report`) in
`agent.history` — this is what lets the API/dashboard show "tools used"
without guessing from raw text, and is also why the cached system prompt
never leaks into a conversation view: it's never part of the history.

### Token usage & caching

Every LLM call is logged under a random `message_id` with its token usage
(prompt/completion/total, including cached-token counts the provider
returns). `report` and `tool_use` history entries carry that `message_id` so
usage can be looked up per message.

---

## Infrastructure / where things are accessible

Everything runs in **one Docker container** built from the `Dockerfile`:

```bash
./run.sh
# = docker build . -t zaplauncher && docker run -it --env-file .env -p 8000:8000 zaplauncher:latest
```

This requires a `.env` file at the repo root with:

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
| Web dashboard | open `dashboard/index.html` directly in a browser (no server needed — it's static HTML/CSS/JS that calls the API via `fetch`); set the API base URL in the top bar if it isn't on `http://localhost:8000` |
| CLI | `docker run -it --env-file .env zaplauncher python main.py` (overrides the container's default command) |

The container's **default command runs the API** (`uvicorn api:app --host
0.0.0.0 --port 8000`), since that's what the dashboard and any external
client need. The CLI is still there for a quick terminal chat, but you have
to ask Docker to run it explicitly instead of the API.

LLM calls go out to Mistral's API (`https://api.mistral.ai/v1`, OpenAI-compatible
client) using the model/endpoint/cache key configured per team.

### Code map

| File | Role |
|---|---|
| `multiagent.py` | Core types (`Team`, `Agent`, `HistoryEntry`) + all `prompt_database` read/write helpers + prompt assembly |
| `tools.py` | The tools agents can `USE` |
| `runtime.py` | In-memory state shared by the CLI and the API: running `teams`, `agents`, `usage_log` |
| `engine.py` | LLM call + USE/REPORT turn loop, shared by the CLI and the API |
| `main.py` | CLI entry point |
| `api.py` | FastAPI app: every route the dashboard (or any client) uses |
| `dashboard/` | Static web dashboard (no build step, no dependencies) |
| `prompt_database/` | All prompt text, organized as described above |

### API surface (see `/docs` for full schemas)

- `GET /prompts/teams[/​{id}]`, `/prompts/personalities[/​{name}]`,
  `/prompts/protocol`, `/prompts/trantorian` — browse preset prompts
- `GET /teams` · `POST /teams` — list/create teams
- `POST /personalities` — create a personality
- `GET /agents/{id}` · `POST /agents` · `PATCH /agents/{id}` — describe,
  create, or live-edit a running agent's prompts
- `GET /agents/{id}/conversation` — structured turn-by-turn history
- `POST /agents/{id}/prompt` — send a message, get back the new history
  entries
- `GET /usage/{message_id}` — token usage for one LLM call
