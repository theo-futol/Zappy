# Bonus — Change Recap & Design Notes

This document recaps every fix and feature added during the July 2026 review
pass of `bonus/`, and explains how each was designed. Components involved:

- **`server/`** — the C++ Zappy game server (serial-turn "wait mode" variant)
- **`GUI/`** — Node bridge (TCP → WebSocket/HTTP) + PixiJS browser viewer
- **`ZapLauncher/`** — Python FastAPI app driving LLM agents over the AI protocol

---

## 1. Correctness fixes

### 1.1 GUI: trantorian statuses were wrong or invisible (`GUI/browser/index.html`)

The bridge already aggregated full player state (position, orientation,
level, **inventory**) from the server's `pipi` snapshots and `pin` replies,
but the browser threw most of it away:

| Problem | Fix |
|---|---|
| `pipi` events only moved the sprite; level and inventory were discarded | `pipi` now updates position, level **and** inventory |
| No `pin` handler at all | Added: inventory-only updates are applied |
| Player-info panel showed only Team/Level/Last Action, and Level went stale while the panel was open | Panel is re-rendered on every status event of the tracked player and now shows **food (color-coded, "starving!" below 3)** and all six stones |
| `players_sync` (1 s resync) ignored level/inventory of already-spawned players | Both are now synced |
| Tile panel listed players by parsing the `Lv.x` text sprite | Players carry a structured `userData.level` / `userData.inventory`; the tile panel shows level and food per player |
| `updatePlayerAction` compared tracked-player ids with `===` across string/number types | Compared as strings |
| `spawnPlayer` assigned `userData` twice (first object silently thrown away) | Single assignment seeding level/inventory from the spawn data |
| Eggs were never displayed (no handlers, and the server never announced them — see 1.3) | Egg sprites: spawned on `enw`, removed on `ebo`/`edi`, restored from `/api/state` on reload, re-attached on map rebuild |
| No way to see what an agent said/heard | Clicking a player now shows its **conversation history**: everything it broadcast (SAID) and every broadcast it heard from others (HEARD, colored by the speaker's team), backed by a 300-entry ring buffer in the bridge's `GameState.broadcasts` so it survives page reloads |
| Speech bubbles could not be turned off | `💬 Bubbles: ON/OFF` toggle button in the overlay (the right-side broadcast log stays) |
| Carried resources shown as text while tile resources use images | The player panel now uses the same convention as the tile panel: resource **images** with the count, and the resource **name on hover** (`title`) — on both panels |

Design note: the browser keeps all live status inside each player container's
`userData` (single source per entity), and every mutation goes through
`setPlayerLevel` / `setPlayerInventory`, which also refresh the info panel
when that player is tracked. This removes the class of "sprite text says X,
panel says Y" bugs.

### 1.2 GUI bridge: dead broken file (`GUI/bridge/config.js`) — deleted

It was syntactically invalid JavaScript (semicolons inside an object
literal), imported `dotenv` which isn't a dependency, and was imported by
nothing. `bridge.js` reads its configuration from the environment directly.

### 1.3 Server: egg lifecycle was invisible to GUIs

The GUI protocol (and the bridge) know `pfk`/`enw`/`ebo`/`edi`, but the
server never emitted them. Now:

- `Fork` (in `Commands::Fork`) pushes `pfk <player>` and
  `enw <egg> <player> <x> <y>` — `Team::addEgg` now returns the new egg's id.
- A connection hatching an egg (`World::addPlayer`) pushes `ebo <egg>`.
- `Eject` destroying the eggs on a tile pushes `edi <egg>` for each
  (new helper `Team::getEggIdsAtPosition`).

Also fixed while in there:

- **`Take` answered `ok` for absent resources**: if the requested item type
  wasn't present in the tile's item list at all, the loop fell through and
  the command still broadcast `pgt` + returned `ok`. It now returns `ko`
  in every not-actually-taken case.
- **`Set` could evaporate a stone**: if the tile's item list lacked the item
  type, the stone was removed from the inventory but never placed. The tile
  entry is now created when missing.
- **`Team::addEgg` ignored `count`** when the tile already had eggs (latent).
- Removed a stray `puts("ok")` debug print executed for *every* command.

### 1.4 Server: broadcasts were timed with CPU time — they (almost) never arrived

Found by the real-server e2e suite: `Broadcast` queues each recipient's
message with a distance-based delay, but both the queue timestamp and the
delivery check used **`std::clock()` — process CPU time**. A mostly idle
server (blocked in `poll()`) accrues CPU time far slower than wall time, so
a "49 ms" delay could take arbitrarily long; in practice other players never
heard anything, silently crippling all LLM-agent coordination.

The message queue now uses `std::chrono::steady_clock` (what every other
timer in the server already uses); delays are real milliseconds. Verified by
`test_broadcast_reaches_other_players` against the real server.

### 1.5 Server: GUI query commands rejected standard requests; bct corrupted by eggs

Found while writing the cross-layer consistency test:

- `ppo #n`, `plv #n`, `pin #n` read `args[1]`, so a standard single-token
  request (`pin #3` → one argument) always answered `ko`; only the
  non-standard two-token form would have worked. All three now parse the
  first argument, accepting both `#n` and bare `n` (`parsePlayerIdArg`
  helper, safe `stoi`).
- `bct X Y` had the same off-by-one (`args[1]`/`args[2]`), and serialized
  *every* entry of the tile's item list. Since `Fork` appends an `EGG`
  entry, any tile that ever hosted an egg produced an 8-count `bct` line —
  which the bridge (expecting exactly 7 quantities) silently dropped,
  freezing that tile's resources in the GUI forever. `bct` now emits exactly
  the 7 resources in protocol order.

### 1.6 ZapLauncher: type/API mismatches and a socket race

- `HistoryEntry.kind` was missing `"llm_response"` (logged by the engine
  since the multi-USE rework) and the `tool_input` field the engine writes.
- The API's `ConversationEntry` response model silently **stripped
  `tool_input`** from every response (FastAPI drops undeclared fields) —
  added, so clients can show full commands like `take food`.
- **Race**: `POST /agents/{id}/prompt` and `/action` could run while the
  autonomous loop was mid-turn on the same TCP socket, interleaving two
  readers. All three entry points now claim the agent via the shared
  `_busy_agents` set; a concurrent call gets **409 Conflict** and the claim
  is always released in a `finally`.
- `/action` now also delivers broadcasts that arrived during the command
  (same behavior as engine turns).
- Dead code removed: `multiagent.build_prompt` (superseded by the engine's
  message assembly) and `fetch_protocol_from_server` (the server has no HTTP
  endpoint) with their now-unused imports.
- README brought back in sync with reality (no `main.py`/`run.sh`; 12 real
  tools, 8-step cap; new endpoints and frontends; testing instructions).

---

## 2. Faster LLM game (without breaking the serial turn loop)

The game speed in wait mode is *sum of per-turn latencies*: server offers a
turn (`waiting N`) → launcher notices → LLM call(s) → command round-trips.
Two levers were pulled (chosen over server-side turn pipelining, which would
change game semantics):

### 2.1 Event-driven turn dispatch (`api.py`)

Before: the autonomous loop woke every **500 ms** and polled each agent
socket — on average +250 ms, worst case +500 ms of dead time added to *every
single turn*, plus the whole game is paused meanwhile (serial mode).

Now the loop `select()`s on all idle agents' sockets (in a worker thread via
`asyncio.to_thread`) and wakes **the moment the server sends anything**;
`poll_waiting()` (a non-consuming `MSG_PEEK`) then confirms it's a turn
notification and the turn starts immediately. The 0.5 s select timeout only
bounds how quickly *newly created* agents join the watch set. Verified by
test: dispatch happens well under the old poll period.

### 2.2 Trimmed LLM context (`engine.py`)

- History window halved (60 → 30 entries): with the serial loop, 60 entries
  spans far more game-time than an agent needs to act coherently.
- Old tool results are truncated: entries older than the last 10 keep only
  their first 160 characters. Stale `look`/`inventory` dumps dominated token
  count while carrying almost no signal (the world has changed since).
  Fresh results keep full detail.

Fewer input tokens = faster time-to-first-token and cheaper calls; the
prompt-cache-friendly prefix (protocol + lore + team) is untouched.

The USE-chaining lever (several `USE` lines per LLM response, already in the
protocol prompt) stays as-is — it was reviewed and the engine executes chains
correctly (verified by test).

### 2.3 Protocol prompt: cooperate through broadcasts (`prompt_database/protocol.txt`)

The prompt now explicitly teaches division of labor, the other big speed
lever (and the one the broadcast-delivery fix of §1.4 finally makes
possible): ritual stones can come from several inventories, so agents are
told to treat broadcasts as the team's shared memory — gather the
*complement* of what teammates announce (never duplicates), claim targets
out loud, converge on a meeting point with K, each `set` their share, and
show up even empty-handed (rituals need same-level bodies as much as
stones). Added to the RESPOND priority, the elevation walkthrough, the
when-to-broadcast list, and the example broadcasts. Note: running agents
keep their old cached prompt — recreate agents or `PUT /prompts/protocol`
(which rebuilds every agent's cached prefix) to apply it live.

---

## 3. Point-of-view modes (three frontend endpoints)

All three are static, build-free pages served **by the ZapLauncher API
itself** (FastAPI `StaticFiles`), so there is a single origin, no CORS
friction, and one port (8000) in Docker. `/` redirects to `/leader/`.

### 3.1 `/leader` — team leader (existing dashboard, relocated)

The previous `dashboard/` is now mounted at `/leader`. Its API base defaults
to `window.location.origin` when served over HTTP (still falls back to
`http://localhost:8000` when opened from disk).

### 3.2 `/player` — play as a client (manual control, isometric vision)

Design choice: **direct manual control, no LLM in the loop.**

- `POST /agents/{id}/control {"manual": true|false}` puts an agent under
  human control: the autonomous dispatcher **skips manual agents** entirely
  (`_manual_agents` set), so the human and the LLM never fight over the
  socket. `/status` exposes the manual set so every frontend can show it.
- The page drives the existing `POST /agents/{id}/action` route (per-action
  busy claim; broadcasts arriving mid-command are delivered with the result).
- Server-side nothing special is needed for turn-taking: in wait mode the
  server's `waiting` lines are skipped by `send_command`, and a manual
  agent that stays idle simply has its turn skipped after the timeout.

**The interface is the agent's own senses, rendered:**

- **Isometric vision**: the latest `look` result is drawn as diamond tiles —
  you at the bottom, rows extending ahead (row count grows with level).
  Tiles show their resources, other Trantorians, and eggs.
- **Same style as the map GUI**: the page reuses the GUI's visual language
  (dark `#111`/`#1a1a2e`, monospace, translucent black panels with
  white/gold borders, `#333` buttons, gold accents, green/red status dots)
  and its actual **sprites** — the grass texture (rotated 45° and squished
  to half height, exactly like the PixiJS ground), the golem for
  Trantorians, and the crystal/bush images for resources (copied into
  `player/assets/`, name on hover). A test asserts the sprites are served.
- **Absolute facing (N/E/S/W)**: the AI protocol never reveals it, so a new
  **`GuiMonitor`** (a background GRAPHIC connection owned by the launcher)
  mirrors the server's GUI event stream: at agent creation it claims the
  matching `pnw` to learn the agent's **player id**, then tracks
  `pipi`/`ppo`/`plv`/`pin`/`pdi`. Exposed as `GET /agents/{id}/state`
  (position, orientation, facing, level, inventory) and shown as a compass.
- **Only actions that can succeed are offered** (computed from the latest
  look/inventory/state):
  - *take* — click a glowing resource on **your** tile (also listed in an
    "On your tile" strip);
  - *set* — click a resource you actually carry in the inventory strip;
  - *walk* — click any visible tile; the protocol's navigation sequence
    (turn / forward…) is queued automatically;
  - *eject* — button appears only when another Trantorian shares your tile;
  - *incantation* — button appears only when your tile satisfies the
    elevation table for your level (stones on the ground + enough bodies);
  - *turn left/right, fork, broadcast* — always valid, always present.
- **Action queue**: actions are chosen at any moment — also while the server
  is still processing the previous one — and are kept as removable chips,
  dispatched strictly in order ("your turn is processing…" spinner on the
  head of the queue).
- **Always up to date**: after the queue drains, the page automatically
  sends `look` + `inventory` (real game commands, visible in the log) and
  refreshes `/state`, so vision, inventory, facing and the contextual
  actions always reflect the server's truth.
- **Spectator mode**: selecting an agent shows its environment and inventory
  *before* taking control — without injecting any command into the running
  agent (which would pollute its LLM context and race its socket). The
  vision is harvested from the agent's **own latest `look`/`inventory`
  results** in its conversation history (the LLM looks constantly, and new
  results stream in through the 2.5 s conversation poll); inventory, facing,
  level and position come cost-free from `/state` (GuiMonitor), refreshed
  every 5 s. In spectator mode everything is read-only: no glowing
  take/set affordances, no click-to-walk — those appear on Take control.

### 3.3 `/playbook` — scripted scenes

Design choice: **JSON files in `prompt_database/playbooks/`** (the
filesystem-is-the-database convention), with steps doubling as assertions:

```json
{
  "name": "smoke-basics",
  "team": "REDS",
  "agents": [{ "id": "scout", "personality": "explorator" }],
  "steps": [
    { "agent": "scout", "tool": "look", "args": "", "expect": "tile0" },
    { "agent": "scout", "tool": "take", "args": "food", "expect": "ok" }
  ]
}
```

- API: `GET /playbooks`, `GET/PUT /playbooks/{name}` (names restricted to
  `[A-Za-z0-9_-]+` — no path traversal), `POST /playbooks/{name}/run`.
- Run semantics: missing agents are created & connected on demand; all scene
  agents are put under manual control **for the duration of the run** (the
  LLM loop must not interleave) and released in a `finally`; steps run
  strictly in order through the same tool layer as everything else.
- A step with `expect` *passes* when the result contains that substring; a
  failed expectation marks the scene failed but **continues** (you want the
  full picture), while hard errors (unknown tool/agent) abort and mark the
  remaining steps `skipped`.
- Steps accept `wait_ms` (capped at 5 s): a pause before the step, needed by
  conversation scenes to let distance-delayed broadcasts arrive before a
  listener asserts on them. Broadcasts arriving during a step are appended
  to its result (`message K, ...`), same as `/action` — a runner bug found
  by the village scene: it originally dropped them.
- Reference heavy scene: **`village-gathering`** — six named villagers
  (aldric/berta/cedric/dora/edmund/farah) wake, converge on the square, and
  hold a signed broadcast conversation (greetings, a confirm-and-answer
  exchange about a linemate find, a village plan echoing the
  verify-understanding culture), 63 steps with 6 `expect: "message"`
  assertions proving the words were actually heard. Passes in ~5 s against
  the real server in classic mode.
- **Wait mode vs scenes**: with the serial-turn server (`--wait-timeout`),
  every step pays the timeout for each idle client's skipped turn — the
  6-villager scene took 13+ minutes. The server image now takes
  `WAIT_TIMEOUT` from the environment (`ZAPPY_WAIT_TIMEOUT` via compose;
  empty = classic mode), so playbook testing runs the fast path:
  `ZAPPY_WAIT_TIMEOUT= ZAPPY_CLIENTS=20 docker compose up -d server`.
- Every step is logged into the agent's history, so a scene is replayable
  and inspectable from the `/leader` view afterwards.
- UI: playbook list, JSON editor with save/new-from-template, run button,
  per-step results table (pass / fail / error / skipped) and a scene verdict.
- The runner makes playbooks a *bonus e2e harness*: the same scene files
  exercise the whole chain launcher → TCP protocol → server.

---

## 4. Relationships & personal memory

Goal: make Trantorians conscious of the *individuals* around them — identify
peers by their signed name + team, hold conversations, and let those bonds
persist.

**Mechanism** (the prompt alone can't do this: the conversation window is 30
entries, so anything social would be forgotten within a few turns):

- New agent tools `remember <fact>` / `forget <number>` writing to the
  previously **unused `Agent.facts_memory` field** — up to 30 facts, oldest
  dropped when full.
- The engine injects the numbered fact list as a system message on **every
  turn** (after the identity header, so the team-shared cacheable prefix is
  untouched). Relationships now outlive the history window.
- `facts_memory` is exposed on `GET /agents/{id}` and shown in the `/leader`
  agent view as a collapsible "Memory" panel that auto-refreshes whenever a
  `remember`/`forget` appears in the conversation.

**Prompting**:

- `protocol.txt` gains a "RELATIONSHIPS" section: every broadcast is signed,
  so build a mental map of who speaks (name, team, level, history, standing);
  address peers *by name*; hold multi-exchange conversations (greet, answer,
  ask back, confirm plans) while continuing to act; prefer proven partners
  when calling for rituals; track rivals too. It documents the memory
  actions with good/bad examples of what deserves a slot (individuals,
  deals, plans — not tile contents or K values).
- `TRANTORIAN.md`: the Communication section's "avoid unnecessary
  conversation" (which contradicted the goal) was replaced with conversation
  etiquette, and a new "Relationships" lore section defines gathering
  partners, ritual crews, friends and rivals — earned through exchanges,
  lost through silence and lies — ending on the practical incentive: strong
  bonds mean rituals assemble in moments instead of ages.

Tested: memory tool behaviors (store/cap/forget/validation), system-message
injection (present, numbered, absent when empty), persistence across turns
via `USE remember`, API exposure, and memory editing through `/action`.

### 4.1 Cross-level helping & verified understanding (`protocol.txt`, `TRANTORIAN.md`)

Two further cooperation layers were added to the prompts:

- **"Helping across levels — nobody gets left behind"**: only same-level
  players count as ritual participants, so when a teammate calls and no
  same-level partner answers, others are told exactly how to still help —
  set stones on the ritual tile (stones work regardless of who set them),
  drop food for busy gatherers, higher-levels guide/equip lower teammates
  upward (six players must reach the top — pulling the team up *is*
  progress toward your own goal), lower-levels offer what they carry. A
  call for help is never left unanswered when either support or leveling
  someone up is possible.
- **"Verify understanding — others can be wrong"**: teammates mishear,
  forget, or act on stale info, and high-level rituals (4–6 players) sink
  on one confused participant. Agents must CONFIRM plans out loud, ASK when
  something is unclear or contradicts what they know, ACKNOWLEDGE
  confirmations explicitly (silence reads as absence), and CORRECT wrong
  stone lists/player counts from the elevation table — everyone on the tile
  echoes the final plan before the initiator casts. `TRANTORIAN.md`'s Team
  Identity section mirrors both: collective Ascension duty ("a teammate
  left behind is your problem too") and "understanding is verified, never
  assumed" — including that *you* may be the one who is wrong.

---

## 5. Tests

Nothing in `bonus/` was tested before. Everything above is now covered by
functional and end-to-end tests, all runnable through the Docker images
already used by the project.

### 5.0 Cross-layer consistency

`test_agent_and_gui_views_agree_on_inventory_and_level` (real-server e2e)
asserts that the same player's **resources and level are seen identically by
the server, the agent, and the GUI protocol**: the agent's `Inventory` reply
must match the GUI's `pin` counts field-by-field, `plv` must report the
expected level, and `ppo` must be on the map. The bridge suite closes the
chain by asserting the browser-facing `GameState` reflects `pipi`/`pin`/`plv`
lines exactly (including that finding this test surfaced — §1.5).

### 5.1 ZapLauncher — pytest (`ZapLauncher/tests/`, 71 tests + 9 real-server e2e)

Run: `docker run --rm -v $PWD/ZapLauncher:/app -w /app -e API_KEY=test
python:trixie bash -c "pip install -q -r requirements-dev.txt && pytest"`

- `conftest.py` — **FakeZappyServer**: a threaded TCP server speaking the
  real AI + GRAPHIC handshake with scriptable replies (multi-line replies
  let tests interleave `message`/`waiting` lines), plus fixtures isolating
  `runtime.agents`, the busy/manual sets and the playbooks dir per test.
- `test_connection.py` — handshake (incl. `ko`), reply routing, broadcast
  queueing + underscore decoding, `waiting` skipping, death handling,
  non-consuming `poll_waiting`.
- `test_tools.py` — look cleaning, broadcast encoding/validation, take/set
  guards, death → `[dead]` + connection cleared, no-server placeholders.
- `test_engine.py` — scripted fake LLM: REPORT ends turn, multi-USE chains
  execute in order, `tool_input` recorded, malformed answers flagged,
  REPORT stops trailing commands, unknown tools fed back as errors,
  max-steps cap, history window, old-result truncation, trigger roles.
- `test_api.py` — status (busy/manual), action logging, **409 on busy**,
  busy release on error, control toggle, agent creation against the fake
  server (incl. no-slots 503), broadcast delivery through `/action`,
  conversation pagination, frontends served, root redirect.
- `test_autonomous.py` — event-driven dispatcher: dispatches on `waiting`,
  skips manual and busy agents, reacts faster than the old 500 ms poll,
  `_run_autonomous_turn` consumes the notification and runs an autonomous
  engine turn.
- `test_playbooks.py` — CRUD roundtrip, name validation, run: agent
  creation, expectations, failed-expect vs hard-error semantics, busy 409,
  manual-set release, and a full **e2e scene** asserting the exact command
  stream received by the fake server.
- `test_gui_monitor.py` — player-id claiming at agent creation, the
  `/agents/{id}/state` route (facing/level/inventory), live updates from
  pushed `pipi`/`plv`/`ppo` events, removal on `pdi`, all-null response
  without a server, distinct ids for concurrent agents. The real-server e2e
  adds `test_gui_monitor_claims_and_mirrors_real_server`, asserting the
  mirror matches the agent's own `Inventory` view.

### 5.2 GUI bridge — `node --test` (`GUI/bridge/test/`, 12 tests)

Run: `docker run --rm -v $PWD/GUI:/gui -w /gui/bridge node:22-alpine
sh -c "npm install && npm test"`

A fake Zappy TCP server feeds protocol lines to a **real bridge process**;
assertions go through the bridge's own `/api/state`. Covers the handshake
(msz/tna/sgt), `pnw`+`pipi` carrying level+inventory, `plv`, `pin`, `ppo`,
`bct`, `enw`/`ebo` egg tracking, `pdi` removal, HTML escaping of team names,
static file serving, and malformed-line resilience.

### 5.3 Real-server e2e — pytest (`tests/test_e2e_real_server.py`)

Runs against the compose-built C++ server (skipped unless
`ZAPPY_E2E_HOST`/`ZAPPY_E2E_PORT` are set — see README §Testing; start the
server with `ZAPPY_CLIENTS=20`, see §5). Covers handshake, movement,
look/inventory formats, **the Take-absent-resource regression**,
Set-without-item, **cross-player broadcast delivery (the CPU-clock
regression of §1.4)**, and the **new egg events** (`pfk`+`enw` on Fork,
`ebo`+`pnw` on connection). All 7 pass against the rebuilt image.

---

## 6. Known limitations / consciously out of scope

- GUIs connecting mid-game don't receive the current egg list (there is no
  egg-listing command in the GUI protocol); they see eggs laid from then on.
  The bridge's `/api/state` does persist eggs for browser reloads.
- **Eggs are lives**: every connection consumes an egg, and neither death
  nor disconnection refunds one — a team accepts only `-c` connections for
  the server's lifetime unless its players Fork. This looks intentional
  ("one egg, one life") and was left as-is, but it means the e2e suite (and
  any long ZapLauncher session with reconnects) needs a generous `-c`
  (`ZAPPY_CLIENTS=20` for the e2e run).
- The browser viewer loads PixiJS/nipplejs from a CDN — it needs internet
  even when everything else is local.
- Server-side turn pipelining (overlapping LLM "thinking" across players)
  was considered and deliberately not done: it changes wait-mode semantics.
- Playbook steps are strictly sequential; no parallel step groups yet.
