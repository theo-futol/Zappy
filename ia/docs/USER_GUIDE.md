# Zappy AI User Guide

This document explains how to run and monitor the Python AI for the Zappy
project. It only mentions the C++ server when it is needed to start a complete
game.

## AI Role

The AI controls autonomous Zappy players. Each player:

- connects to the server with a team name;
- prioritizes food when its stock is low;
- collects stones required for its next level;
- coordinates incantations with players from the same team;
- forks eggs to keep client slots available;
- keeps running until death, server shutdown, or victory.

Victory is detected by the server when a team has at least six level-8 players.
At protocol level, the server sends `seg <team>` to graphical clients, and the
AI client parses that message as a game-over event.

## Start A Game

From the repository root:

```bash
make
./zappy_server -p 4242 -x 42 -y 42 -n Team1 Team2 -c 5 -f 1000
```

In another terminal:

```bash
python3 -m ia.launcher --host 127.0.0.1 --port 4242 --teams Team1 Team2 --target-per-team 25
```

The launcher keeps the requested number of active clients per team. If a player
dies, it starts another client when a slot is available.

## Run One Client

For an isolated test:

```bash
make zappy_ai
./zappy_ai -p 4242 -n Team1 -h localhost
```

Useful options:

- `-p`, `--port`: server TCP port, defaults to `4242`;
- `-n`, `--team`: Zappy team name, required;
- `-h`, `--host`: server TCP host, defaults to `localhost`;
- `--quiet`: disables client stdout logs.

Because the subject uses `-h` for the host, help is available with `--help`.

## Run Multiple Clients

Recommended command:

```bash
python3 -m ia.launcher --host 127.0.0.1 --port 4242 --teams Team1 Team2 --target-per-team 25
```

Useful launcher options:

- `--teams Team1 Team2`: teams to supervise;
- `--target-per-team 25`: desired number of active clients per team;
- `--target-per-team 0`: unbounded mode, the launcher keeps trying to fill any future slot;
- `--spawn-interval 0.35`: minimum delay between two spawns for the same team;
- `--retry-interval 3`: delay after a normal or abnormal client exit;
- `--no-slot-retry-interval 15`: initial delay when the server refuses a team;
- `--show-client-logs`: keeps client logs visible in the launcher's terminal.

Without `--show-client-logs`, the launcher hides client stdout and only reads
stderr to detect deaths (`dead level=X food=Y`) and slot-refusal errors.

## Read Logs

The client tags its logs with the team and process id:

```text
[Team1:47388] [send] Look
[Team1:47388] [recv] [food 8, linemate 1, ...]
```

If logs are redirected to `client.log`, a single player can be isolated with:

```bash
grep "\[Team1:47388\]" client.log
```

Useful lines:

- `[send] Broadcast ...`: the player announces that it is ready for an incantation;
- `[recv] message K, ...`: the player received a broadcast;
- `[send] Incantation`: incantation attempt;
- `[recv] Elevation underway`: ritual accepted and player frozen;
- `[recv] Current level: N`: level-up notification;
- `dead level=N food=Y`: client death, printed on stderr.

## Monitor Progress With The Server Dump

The C++ server can generate a level dump in `src/server/level.txt` when that
instrumentation is enabled. Each block shows:

- the tick;
- the number of players per level;
- alive, dead, and frozen players;
- each player's position, rotation, and inventory.

Quick checks:

```bash
tail -n 120 src/server/level.txt
grep "level 8:" src/server/level.txt | tail
grep "Current level: 8" client.log | tail
```

When diagnosing a plateau, check the largest cluster of same-level players from
the same team. If many players have stones but no cluster reaches the required
player count, the problem is coordination rather than gathering.

## Expected Game Behavior

Early levels should progress quickly, either solo or in small groups. Levels 4
to 6 require increasingly strict coordination. Players broadcast rally pings
when they carry at least one useful stone and enough food.

For levels that require six players, the AI avoids stopping on simple pairs. It
keeps groups of three or more stable and keeps stones in inventory until the full
quorum of six is present. This prevents resources from being scattered across
multiple tiles.

## Local Validation

Before a long game:

```bash
python3 -m py_compile ia/*.py ia/utils/*.py
python3 ia/test.py
```

The manual test suite must print `All scenarios passed.`.

## Known Limits

- Rally navigation uses sound directions from the protocol, which are coarse.
- Vision does not expose the level of visible players; the AI filters by level
  through broadcasts, not through `Look`.
- The AI only cooperates with its own team.
- Full logs can become large when `--show-client-logs` is enabled or stdout is
  redirected during a long game.
