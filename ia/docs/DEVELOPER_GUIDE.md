# Zappy AI Developer Guide

This document describes the Python architecture under `ia/`, the contracts
between modules, and the points to check before changing the strategy.

## Overview

The AI is intentionally flat:

```text
ia/
  client.py             synchronous TCP connection and runtime protocol handling
  launcher.py           multi-process supervisor
  strategy.py           FSM and game decisions
  observation.py        fresh Look + Inventory snapshot for each turn
  broadcast.py          rally ping format and sound navigation helpers
  parsing.py            server line parsing
  config.py             strategy constants
  utils/model_utils.py  incantation rules and pure helpers
  test.py               manual scenarios without a server
```

The C++ server is authoritative. The Python client does not simulate the world:
it reads `Look`, `Inventory`, and received messages, then chooses simple
commands.

## Root Packaging

The root Makefile generates the subject binary:

```bash
make zappy_ai
./zappy_ai -p 4242 -n Team1 -h localhost
```

`zappy_ai` is a small shell launcher that runs `python3 -m ia.client` from the
repository root. The `PYTHON` environment variable can select another
interpreter:

```bash
PYTHON=python3.12 make zappy_ai
```

The client also keeps the long options (`--host`, `--port`, `--team`) so the
multi-process launcher can continue to call it directly.

## Player Loop

`ZappyAIClient.run()` creates a `Strategy`, then calls `strategy.step()` until
death or game over.

One `step()` always:

1. applies a server-pushed level (`pending_level`) if the player participated in
   another player's incantation;
2. rebuilds an `Observation` from `Look` and `Inventory`;
3. ages known rally pings;
4. integrates broadcasts received during previous commands;
5. decrements cooldowns;
6. selects an FSM state;
7. executes that state's body.

This avoids keeping complex local state that could drift away from the server.

## Synchronous TCP Client

`client.py` sends one command and waits for its terminal response. It does not
use the protocol's 10-command pipeline.

Benefits:

- each log line is easy to follow;
- the code does not manage a local command queue;
- observation is always rebuilt from server data.

Important details:

- Broadcasts received while waiting for a command are stored in
  `_pending_messages`, then consumed by `Observation`.
- `Current level: N` can be unsolicited: the server sends it to every
  participant of a successful incantation.
- During a ritual started by another player, the server can freeze a player
  while one of its commands is pending. `_frozen_for_ritual` prevents the
  ritual's `ko` or `Current level` from being confused with that command's real
  response.
- The `seg <team>` message is parsed as `game_end`.

## Observation

`Observation` is the only world object read by the strategy.

Main fields:

- `tiles`: normalized visible tiles with `index`, `x`, `y`, `distance`, and
  `items`;
- `inv`: normalized inventory returned by `Inventory`;
- `food`: shortcut for `inv["food"]`;
- `current_counts`: item counts on the current tile;
- `needed_stones`: stones still missing from inventory for the current level;
- `ready_for_elevation`: true when inventory covers every required stone;
- `players_here`: number of `player` entries on the current tile;
- `messages`: broadcasts received since the previous observation.

The current tile always contains at least the player itself. `parsing.py` adds
`player` if the server does not explicitly return it in `Look`.

## Incantation Rules

The AI source of truth is `LEVEL_REQUIREMENTS` in `ia/utils/model_utils.py`:

| Current level | Players | Stones |
| --- | ---: | --- |
| 1 | 1 | `linemate:1` |
| 2 | 2 | `linemate:1 deraumere:1 sibur:1` |
| 3 | 2 | `linemate:2 sibur:1 phiras:2` |
| 4 | 4 | `linemate:1 deraumere:1 sibur:2 phiras:1` |
| 5 | 4 | `linemate:1 deraumere:2 sibur:1 mendiane:3` |
| 6 | 6 | `linemate:1 deraumere:2 sibur:3 phiras:1` |
| 7 | 6 | `linemate:2 deraumere:2 sibur:2 mendiane:2 phiras:2 thystame:1` |

Level 8 has no next incantation.

## Rally Broadcasts

The only broadcast format emitted by the AI is:

```text
level|team|token|stone:qty+stone:qty
```

Examples:

```text
6|Team1|2cbfcf4e|linemate:1+phiras:1
6|Team1|09b039c4|none
```

Meaning:

- `level`: sender's current level;
- `team`: sender's team;
- `token`: random local identifier, stable for the client's lifetime;
- resources: useful contribution carried by the sender for the current step.

Messages are ignored when the team or level does not match, or when the token is
the current player's own token.

`ready_seen` stores one entry per token:

```python
{
    "direction": int,
    "age": int,
    "resources": dict[str, int],
}
```

A ping expires after `RALLY_CALL_EXPIRY_TURNS`.

## Incantation Coordination

The strategy moves toward a remote rally only if:

- it is "ready": enough food and at least one useful stone;
- it knows enough ready players to reach the required quorum;
- the sum of announced contributions covers the required stones.

The rally target is selected by deterministic consensus:

```python
min([*ready_seen, self.token])
```

Every player with the same token cohort therefore chooses the same target
without a global election.

Once rallying, the target stays locked. Fresher pings do not steal the lock. If
the target expires, the strategy chooses another known token or eventually gives
up.

## Sound Navigation

`message K, ...` directions are relative to the player's orientation at the exact
moment the message is received. Replaying the same `Left/Forward` or
`Right/Forward` pair every turn would make the player spin.

Applied rule:

- if the target ping arrived during this turn (`age == 0`), use
  `build_plan_from_sound_direction(K)`;
- otherwise keep moving `Forward` on the already corrected heading.

## Six-Player Levels

Levels 6->7 and 7->8 require six players. The strategy has two specific
safeguards:

- a local group must contain at least `required // 2`, meaning 3 players, before
  becoming a stable camp;
- while waiting, stones are not dropped until all six players are on the tile.

Without this, the AI forms too many two-player micro-clusters, drops stones in
several places, then broadcasts `none` because its contribution is no longer in
inventory.

## Forks And Slots

`_maybe_fork()` forks only if:

- fork cooldown is zero;
- food is above `FORK_FOOD_THRESHOLD`;
- `Connect_nbr` returns `0`.

If `Connect_nbr > 0`, an egg or slot is already available. Forking more would
mostly accumulate unused eggs when the launcher limits the number of active
clients.

## Launcher

`launcher.py` supervises several `python -m ia.client` processes.

It:

- maintains a target number of active clients per team;
- respawns after death or exit;
- applies a backoff when the server replies `Equipe refusee par le serveur`;
- stops when a client exits with `GAME_OVER_EXIT_CODE`.

Note: `--objective`, `--inventory-refresh`, and `--max-actions` are parsed for
historical compatibility, but are not passed to the current client.

## Tests

Quick validation:

```bash
python3 -m py_compile ia/*.py ia/utils/*.py
python3 ia/test.py
```

`ia/test.py` uses a `FakeClient`. The tests validate FSM decisions, not the
network, broadcast timings, or server freeze behavior. A real run against the
C++ server is still required after changes to coordination logic.

## Diagnostic Method

To diagnose a plateau:

1. keep client logs with `--show-client-logs` or stdout redirection;
2. read `src/server/level.txt` to count players by level and cluster;
3. check whether resources are available in inventories;
4. isolate one player by `[Team:PID]` tag in `client.log`;
5. compare received broadcasts, expected state, and sent commands.

Typical indicators:

- many ready players but no cluster: rallying problem;
- enough players clustered but no `Incantation`: stones are not dropped or level
  is wrong;
- `Incantation` then `ko`: the server did not validate conditions at ritual
  start or end;
- deaths while rallying heavily: survival must never be bypassed.

## Change Rules

- Keep `parsing.py` pure and robust: it should accept only known protocol lines.
- Avoid adding global coordination state.
- Do not drop stones before there is a clear reason to do so.
- Add a scenario in `ia/test.py` for any strategy regression.
- Validate against the C++ server when a change touches incantations,
  broadcasts, or freeze handling.
