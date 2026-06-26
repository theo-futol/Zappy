# Simulation

`src/server/Simulation/World/{World,Items}.{hpp,cpp}`, `Simulation/Player/{Player,Teams}.{hpp,cpp}`, `Simulation/Player/Inventory/Inventory.{hpp,cpp}`

The Simulation layer is the authoritative game state and the rules that act on it: the map and its resources, the players moving around it, their inventories, and the teams that own connection slots and eggs. None of these classes touch sockets directly — [`Commands`](Commands.md) is the only caller that mutates them in response to protocol messages, and [`ClientHandler`](ClientHandler.md) drives their periodic ticks (resource regeneration, starvation).

## World

`World` owns everything: the map (`Map = std::vector<std::vector<tile>>`, indexed `[x][y]`), every `Player` (`std::vector<std::unique_ptr<Player>>`), and every `Team` (`std::vector<std::shared_ptr<Team>>`). A `tile` holds non-owning `Player *` pointers (ownership stays with `World::_players`) plus a `std::vector<std::pair<ItemType, int>>` pre-seeded with a zero count for every resource type (`FOOD` through `THYSTAME`), and an `_incantationInProgress` flag used to serialize elevation rituals per tile.

**Resource generation** — `ressourcePassiveGeneration()` runs on a fixed schedule from `ClientHandler` (every `RESOURCE_INTERVAL_TIME_UNITS` ticks). For each resource type it computes `totalTiles * density` (density is hardcoded: food 0.5, linemate 0.3, deraumere 0.5, sibur 0.1, mendiane 0.1, phiras 0.08, thystame 0.05) and drops that many units onto random tiles, one at a time.

**Food / starvation** — `foodCheck()` is the other periodic tick (every `CYCLE_TO_DIE` ticks): for every player, if their `FOOD` count is `0`, they are marked `PlayerState::DEAD`, a `pdi` event is pushed to the GUI broadcast queue, and their fd is collected; otherwise one food unit is consumed. All collected fds are then passed to `removePlayer()`. The returned fd list lets `ClientHandler` flag those clients `ClientType::DEAD` so their next due command replies `"dead\n"` instead of executing.

**Elevation / incantation rules** — a static table (`elevationRequirements`, indexed by level 0–7) lists, for each level, the minimum number of same-level players that must stand on the tile and the stones required:

| From level | Players needed | Stones |
|---|---|---|
| 1 | 1 | 1 linemate |
| 2 | 2 | 1 linemate, 1 deraumere, 1 sibur |
| 3 | 2 | 2 linemate, 1 sibur, 2 phiras |
| 4 | 4 | 1 linemate, 1 deraumere, 2 sibur, 1 phiras |
| 5 | 4 | 1 linemate, 2 deraumere, 1 sibur, 3 mendiane |
| 6 | 6 | 1 linemate, 2 deraumere, 3 sibur, 1 phiras |
| 7 | 6 | 2 linemate, 2 deraumere, 2 sibur, 2 mendiane, 2 phiras, 1 thystame |

`isIncantationValid(x, y, level)` checks both conditions against the tile's current state; `getPlayersOnTileAtLevel()` filters `_players` by position and exact level; `removeIncantationStones()` subtracts the consumed stones (floored at 0). All of this is invoked from `Commands::beginIncantation` / `Commands::Incantation` — see [Commands.md](Commands.md).

**Win condition** — `checkWinningCondition()`: any team with `_slotsOccupied >= 6` whose alive members include at least 6 players at level `>= 8` wins; pushes a `seg <teamName>` GUI event and sets `Team::_hasWin`. `ClientHandler` stops the server when this returns `true`.

**Players, teams, tiles** — `addPlayer(fd, teamName)` looks up the team, increments its occupied-slot count, constructs the `Player` at the default spawn `(0,0)`, and registers it onto that tile. `removePlayer(fd)` removes it from its tile and from `_players`, freeing its team slot (guarded against double-freeing, since `Player::setState(DEAD)` already frees the slot — see below). `getTileAt()` is overloaded by player fd or by raw `position`, returning `nullptr` for out-of-bounds coordinates. `setTileAt(pos, type, count)` sets an absolute resource count on a tile (used by `Fork`/`Eject` to place/clear eggs).

**Broadcast queue** — `World` holds a non-owning `std::queue<std::string> *_broadcastQueue`, wired in by [`Core::setWorldBroadcast()`](Core.md). Several `World` methods (`foodCheck`, `checkWinningCondition`) push GUI-protocol events directly; most other game events are instead pushed by [`Commands`](Commands.md), which holds the same queue pointer.

## Player

A `Player` is keyed to its client by the socket fd (`_fd`, *not* owned — closing belongs to `Client`). State:

- `_pos` / `rotation` — current tile and facing (`Degrees`, see [Utils.md](Utils.md)); both wrap toroidally at the map edges.
- `_level` — starts at 1, incremented one at a time by `levelUp()` (called once per successful incantation participant).
- `_team` — `shared_ptr<Team>`, shared with `World::_teams` and used to track/free connection slots.
- `_inventory` — an [`Inventory`](#inventory), seeded with 10 `FOOD` at construction.
- `_state` — `PlayerState::PENDING` at construction (never explicitly transitioned to `ALIVE` in the current code — see note below), `DEAD` once starved or ejected from the game.
- `_frozenUntil` — a `steady_clock::time_point`; while in the future, `isFrozen()` is `true` and [`CommandParser`](CommandParser.md) will not execute any of this player's queued commands. Set by `Commands::beginIncantation` for the duration of a ritual.
- `_messagesToSend` — the delayed-broadcast queue: each entry pairs a message body with a list of `((sendClock, delayMs), receiverFd)` tuples, since the same broadcast text can be in flight to several receivers with different remaining delays.

**Movement** — `nextPosition(mapSize)` computes the tile in front of the player along its facing, wrapping at the edges; `move()` applies it; `setPosition(x, y, mapSize)` is the shared wrap-around logic both use.

**Direction/distance to another tile or player** — `getDirectionTo()` computes the bearing from this player to a target using `atan2`, corrected for the shortest wrap-around path on each axis, then snaps it to the nearest 45°-`Degrees` value via `Direction::getNearestDirection`. `getDistanceTo()` computes the toroidal Euclidean distance the same way. Both are used by `Broadcast` to determine the per-receiver direction sector and delivery delay.

**Delayed messaging** — `addMessageToQueue(message, timeNeeded, receiverFd)` either appends to an existing entry for the same message text or creates a new one, stamping the current `std::clock()`. `sortQueueByTimeNeeded()` orders each entry's delivery list by remaining delay so `sendMessageToClient()` can stop at the first not-yet-due entry instead of scanning the whole list every tick. `sendMessageToClient()` (called every loop iteration via [`ClientHandler::broadcastMessageToClients()`](ClientHandler.md)) flushes and `send()`s every entry whose delay has elapsed, removing exhausted message groups.

> Note: `setState(DEAD)` frees the team slot as a side effect (`_team->removePlayer()`), which is why `World::removePlayer()` checks `getState() != PlayerState::DEAD` before calling `removePlayer()` again — to avoid freeing the same slot twice. `PlayerState::ALIVE` exists in the enum but nothing in the current code transitions a player into it from `PENDING`.

## Teams (`Teams.hpp`)

`Team` is a plain struct (not a class) tracking `_slotsAvailable` / `_slotsOccupied` (a free slot is the right for one more AI to connect) and `_eggs` (`vector<pair<position, vector<int>>>`, grouping egg IDs by the tile they were laid on; IDs come from a process-wide static counter `eggID`). `addEgg()` increments `_slotsAvailable` and appends an ID to the matching tile's list (or creates a new tile entry). `removeEgg(pos, count, eggID)` supports three modes: remove by specific egg ID, remove `count` eggs from a tile, or (`count == -1`) clear every egg on a tile at once — used by `Eject` to destroy all eggs on an ejected tile. `hasEggAtPosition()` and `getEggs()` are read-only helpers consumed by `Commands::Eject`.

## Inventory

A thin wrapper around `std::vector<int>`, sized `ItemType::THYSTAME + 1` and indexed directly by `ItemType` (including `EGG`'s slot, though eggs are tracked on tiles/teams, not in player inventories in practice). `addItem`/`removeItem` clamp out-of-range types as no-ops and clamp removal at zero (no negative counts). `checkInventory()` formats the AI protocol's `Inventory` command reply: `"[food <n>, linemate <n>, ..., thystame <n>]\n"`, in `ItemType` declaration order — which is why `ItemType`'s enum order in [`Items.hpp`](#worlditemshpp) matters: it is the wire order. `getItemCount` is overloaded to accept either an `ItemType` or its protocol string name (via `stringToItemType`).

## Items (`World/Items.hpp`)

Defines `ItemType` (`FOOD, LINEMATE, DERAUMERE, SIBUR, MENDIANE, PHIRAS, THYSTAME, EGG, UNKNOWN`) and the two conversion helpers `itemTypeToString`/`stringToItemType` used everywhere a protocol command needs to translate between the wire's lowercase names and the enum (`Take`, `Set`, `Look`, `Inventory`'s reply, tile pre-seeding in `tile`'s constructor).

## How they collaborate

`World` is the single source of truth and owner of `Player`s and tile state; `Player` owns its own `Inventory` and a reference (`shared_ptr`) to its `Team`; `Team` tracks slots and eggs independently of any specific player. [`Commands`](Commands.md) is the orchestration layer: it never duplicates state, it only reads/mutates `World`/`Player`/`Inventory`/`Team` in response to a parsed protocol command, and reports the result either as a direct reply string or as an event pushed to the broadcast queue for GUIs.
