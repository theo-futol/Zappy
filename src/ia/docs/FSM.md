# Zappy AI FSM

The strategy is a finite-state machine implemented in `ia/strategy.py`. Every
turn rebuilds a fresh observation, selects exactly one state, then executes that
state's commands.

## States

```text
SURVIVING  -> urgently find or take food
GATHERING  -> collect food/stones and announce readiness
JOINING    -> move toward a rally point
WAITING    -> hold a rally position and attract others
INCANTING  -> drop missing stones and launch Incantation
EXPLORING  -> default movement when no target is visible
```

`EXPLORING` mostly exists as a reusable action body. In practice,
`_select_state()` returns `GATHERING` by default, and `GATHERING` calls
`_do_exploring()` when nothing useful is visible.

## Global Priority

State selection follows this order:

1. survival;
2. creation or continuation of a rally;
3. incantation if conditions are satisfied;
4. collection.

Food always wins. If `food <= SURVIVAL_FOOD_THRESHOLD`, the player clears its
active rally and enters `SURVIVING`.

## Full Turn Cycle

```text
pending_level?
  yes -> update level, clear rally

Observation = Look + Inventory + received messages
known ready messages age
current-turn messages are integrated
cooldowns decrease

state = _select_state(obs)
_run_state(obs)
```

State selection only reads the observation and a few internal fields:

- `level`;
- `ready_seen`;
- `rallying`;
- `rally_turns`;
- `target_token`;
- cooldowns.

## SURVIVING

Entry condition:

```python
obs.food <= SURVIVAL_FOOD_THRESHOLD
```

Actions:

1. if `food` is on the current tile: `Take food`;
2. otherwise, if food is visible: move toward it;
3. otherwise, explore.

Side effect: any active rally is abandoned. A dead player cannot help an
incantation, so survival must break collective goals.

## GATHERING

Default state when the player is not in danger and is not committed to a rally.

Secondary actions:

1. `_maybe_fork(obs)`;
2. if the player is ready, `_send_ready_ping(obs)`.

Ready definition:

```python
food >= GATHER_FOOD_THRESHOLD
and at least one useful next-level stone in inventory
```

Main action:

1. if `food < GATHER_FOOD_THRESHOLD`, prioritize food;
2. otherwise, take a candidate resource on the current tile;
3. otherwise, move toward a visible candidate resource;
4. otherwise, explore.

Candidate resources are:

```python
obs.needed_stones | {"food"}
```

Stone priority is:

```text
thystame > phiras > mendiane > sibur > deraumere > linemate
```

## Rally Triggering

A rally starts in two ways.

### Local Rally

If the player is already on a tile with enough players to form a stable local
group:

```python
obs.players_here >= local_group_threshold
```

Then:

```python
rallying = True
target_token = self.token
rally_turns = 0
```

Thresholds:

- levels requiring fewer than 6 players: local threshold = 2;
- levels requiring 6 players: local threshold = 3.

The threshold of 3 at high levels prevents the team from freezing into pairs
that are too small.

### Remote Rally

If the player is ready and known broadcasts are sufficient:

```python
len(ready_seen) + 1 >= required_players
pooled_resources >= required_stones
```

Then the player locks a target:

```python
target_token = min([*ready_seen, self.token])
```

`min()` provides deterministic consensus: no global leader, no vote, and no
extra message.

## JOINING

Entry condition:

- `rallying == True`;
- the player has not arrived;
- the timeout is not exceeded.

Actions:

1. take food if it is already on the tile;
2. take a useful stone if it is already on the tile;
3. follow the target's sound direction.

Navigation:

- if the target ping is fresh (`age == 0`), apply the sound plan;
- otherwise, move straight forward.

Current table:

| Direction | Commands |
| ---: | --- |
| 0 | none |
| 1 | `Forward` |
| 2 | `Left`, `Forward` |
| 3 | `Left`, `Forward` |
| 4 | `Left`, `Left`, `Forward` |
| 5 | `Left`, `Left`, `Forward` |
| 6 | `Right`, `Right`, `Forward` |
| 7 | `Right`, `Forward` |
| 8 | `Right`, `Forward` |

Fresh-ping reason: sound direction is relative to the orientation at receive
time. Reapplying an old rotation every turn makes the player spin instead of
converge.

## Arrival At A Rally

A player is considered arrived if:

```python
direction is None
or direction == 0
or partial_group
```

`partial_group` means:

```python
obs.players_here >= local_group_threshold
and not fresh_target_bearing
```

So a fresh ping keeps priority: if the target has just been heard, the player
keeps correcting its route instead of stopping too early on a group it crossed.

## WAITING

Entry condition:

- the player has arrived;
- the full quorum is not ready for `INCANTING`.

Actions:

1. take food on the tile when possible;
2. keep broadcasting its position;
3. drop useful stones, except at high-level six-player steps.

High-level exception:

```python
if required_players >= 6 and obs.players_here < required_players:
    return
```

For six-player steps, stones stay in inventory until all six players are
present. This keeps contributions visible in broadcasts and avoids abandoned
resources across several camps.

## INCANTING

Entry condition:

```python
_ready_for_incantation(obs)
and obs.players_here >= required_players
```

`_ready_for_incantation()` accepts two cases:

- the player's inventory covers every stone;
- the stones are already on the current tile.

Actions:

1. drop missing stones on the tile;
2. send `Incantation`;
3. if the server returns `Current level: N`, update `level`;
4. clear the rally.

If the server returns `ko`, the strategy does not force a local correction. The
next turn rebuilds an observation and reevaluates conditions.

## EXPLORING

Fallback action:

```text
Forward, Forward, Forward, Left, Forward, Forward, Forward, Right, ...
```

The player moves forward most of the time and alternates turns every
`EXPLORE_TURN_INTERVAL` turns to avoid infinite straight lines.

## Timeouts And Expiration

Each known ping has an age. On each turn:

```python
age += 1
remove if age + 1 > RALLY_CALL_EXPIRY_TURNS
```

An active rally is abandoned if:

```python
rally_turns > _rally_give_up_limit(required)
```

Limits:

- normal step: `RALLY_GIVE_UP_TURNS`;
- six-player step: `RALLY_GIVE_UP_TURNS * 2`.

The high-level multiplier gives the last players time to arrive without making
small steps stall for too long.

## Main Transitions

```text
any state
  food <= survival threshold
  -> SURVIVING

GATHERING
  stable local group
  -> WAITING or INCANTING

GATHERING
  complete ready cohort at distance
  -> JOINING

JOINING
  target reached / partial stable group
  -> WAITING

WAITING
  players + stones ready
  -> INCANTING

INCANTING
  success
  -> GATHERING at next level

INCANTING
  failure
  -> reevaluate next turn

SURVIVING
  food above threshold on next turn
  -> GATHERING or rally depending on observation
```

## Important Invariants

- A player never chases a rally while it is in starvation danger.
- A player never converges toward a group that does not already cover the quorum
  and announced stones.
- The rally target does not change just because a fresher ping arrives.
- Broadcasts from other teams are ignored.
- Broadcasts from other levels are ignored.
- Six-player steps preserve clusters of three or more, not simple pairs.
- High-level stones stay carried until the full quorum is present.

## Why This FSM Works

The hard part of Zappy is not choosing an optimal action on every tick; it is
avoiding collective deadlocks:

- too much stone priority kills players from hunger;
- waiting for one player to own every stone is too slow;
- changing target at every broadcast prevents convergence;
- stopping on every pair fragments levels 6 and 7;
- dropping stones too early makes contributions invisible.

The FSM handles these issues with simple rules:

- absolute survival priority;
- contribution pooling through broadcasts;
- deterministic target consensus;
- target locking;
- sound navigation corrected only on fresh pings;
- stricter local consolidation at high levels;
- late stone dropping for six-player rituals.
