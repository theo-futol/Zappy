# Commands

`src/server/Network/ClientHandler/Commands/Commands.hpp` and `Commands/AI/*.cpp`, `Commands/GUI/*.cpp`

`Commands` implements the *effect* of every protocol verb on the [`World`](Simulation.md): one shared `Commands` instance (constructed once per `CommandParser`, but operating on the same `World *` and broadcast queue for every client) backs every connected client. Each method takes the parsed argument vector and the calling [`Client`](Client.md), mutates the world if needed, and returns the reply string to send back to that client (for AI commands) or simply formats a query result (for graphic commands); side effects meant for *other* clients go through the shared broadcast queue rather than the return value.

There is no `Command` base class or per-command class hierarchy in this codebase — despite the directory name, each "command" is just one method on `Commands`, implemented in its own `.cpp` file purely for organization (one file per verb, split into `AI/` and `GUI/` subfolders). [`CommandParser`](CommandParser.md) is what maps a protocol command name to the matching `Commands::<Name>` method.

## AI commands (player actions)

These mutate player/world state and are subject to the duration-based scheduling described in [CommandParser.md](CommandParser.md).

### Forward / Right / Left (`Move.cpp`)
- `Forward`: moves the player one tile in its current facing (`Player::move`, wrap-around/toroidal map), updates which tile the player is registered on (`World::removePlayerFromTile` / `addPlayerToTile`). Replies `"ok\n"`, or `"dead\n"` if the player no longer exists.
- `Right` / `Left`: rotate the player ±90° (`(rotation ± 90) % 360`, with `Left` wrapping `0 → 270`). Always replies `"ok\n"` (or `"dead\n"`).

### Look (`Look.cpp`)
Builds the vision cone for the player's current level: for each ring `i` from `0` to `level` and each tile `j` in that ring, computes the tile position relative to facing (`NORTH`/`EAST`/`SOUTH`/`WEST` each map `(i, j)` differently), wraps it into map bounds, and lists `"<item>:<count>"` for every resource on that tile plus `"player <n>"` if other players share it. Tiles are comma-separated and the whole thing is wrapped in `[...]`. Replies `"ko\n"` if the calling fd has no player.

### Inventory (`Inventory.cpp` — note: defined as `getInventory()`)
Returns `Player::getInventory().checkInventory()`, i.e. `"[food <n>, linemate <n>, ...]\n"` in the protocol's fixed resource order. `"ko\n"` if no player.

### Connect_nbr (`Connect.cpp`)
Returns the calling player's team's remaining free slots (`Team::getAvailableSlots()`) as a bare number. `"ko\n"` if no player.

### Broadcast (`Broadcast.cpp`)
For every other player in the world, computes the direction (in 1–8 sectors, `getDirectionTo(...) / 45 + 1`) the message *would appear to come from* for that receiver, and queues a delayed `"message <dir>, <text>\n"` to be delivered to them after a travel-time delay (`distance * BROADCAST_MESSAGE_TIME_PER_TILE`, see [Player.md](Simulation.md)). Also pushes a `pbc` event (player fd + text) onto the GUI broadcast queue. Replies `"ko\n"` if there's no player or the text argument is missing/empty, `"ok\n"` otherwise.

### Eject (`Eject.cpp`)
Pushes every other player standing on the same tile to the tile in front of the ejecting player (toroidal wrap), and destroys any eggs on the tile (`Team::removeEgg(pos, -1, -1)` clears all). Every player on the tile (eject target or not) first receives an `"eject: <rotation>\n"` notice via `World::sendMessageToPlayersThatAreOnTile`. Pushes a `pex` GUI event. Replies `"ok\n"` only if something was actually ejected (a player or an egg), `"ko\n"` otherwise.

### Take / Set (`Items.cpp`)
- `Take`: if the named resource has at least one unit on the current tile, decrements the tile count and increments the player's inventory; pushes a `pgt` GUI event. `"ko\n"` on a missing/unknown item, a missing tile, or zero stock.
- `Set`: the inverse — if the player's inventory has at least one unit, removes it and increments the tile's count; pushes a `pdr` GUI event. `"ko\n"` if the player has none of that resource.

### Fork / Incantation (`Evolution.cpp`)
- `Fork`: lays one egg on the player's current tile (`World::setTileAt(pos, EGG, 1)`, `Team::addEgg`), opening a new connection slot for the team. Always `"ok\n"` (or `"ko\n"` if no player).
- `beginIncantation` (called from [`CommandParser::feed()`](CommandParser.md), not directly from the protocol): validates that no ritual is already running on the tile and that `World::isIncantationValid()` passes for the initiator's level; if not, replies `"ko\n"` immediately and returns `false` (so the command is never actually queued — see CommandParser). On success, marks the tile `_incantationInProgress = true`, broadcasts a `pic` GUI event listing every participating fd, sends each participant `"Elevation underway\n"`, and freezes them (`Player::setFrozenUntil(endTime)`) so they cannot act until the ritual resolves.
- `Incantation`: runs once the 300-time-unit duration has elapsed (the queued command itself). Re-checks `isIncantationValid()` (resources/players may have changed during the wait) — if it now fails, broadcasts `pie x y 0` (failure) and replies `"ko\n"`. If it still holds, consumes the required stones (`World::removeIncantationStones`), levels up every participant, broadcasts `plv` per participant and sends each `"Current level: <n>\n"`, then broadcasts `pie x y 1` (success). Returns an empty string to the initiator (the per-participant `"Current level\n"` message already covers the reply).

## Graphic (GUI) commands

These are pure queries with no scheduling delay; most take the form `"<verb> <args...>\n"` mirroring the line the GUI sent. They never mutate state except `Sgt`/`Sst`.

| Command | File | Behaviour |
|---|---|---|
| `Msz` | `Msz.cpp` | Returns map dimensions: `"<width> <height>\n"`. |
| `Bct` | `Bct.cpp` | Returns one tile's resource counts: `"bct <x> <y> <food> <linemate> ... <thystame>\n"`. `"ko\n"` on bad args or out-of-range coordinates. |
| `Mct` | `Mct.cpp` | Returns the concatenation of `Bct` for every tile on the map (full map dump). |
| `Tna` | `Tna.cpp` | Returns one `"tna <teamName>\n"` line per registered team. |
| `Ppo` | `Ppo.cpp` | Returns a player's position+orientation: `"ppo <fd> <x> <y> <rotation>\n"`. `"ko\n"` if the fd argument doesn't match a player. |
| `Plv` | `Plv.cpp` | Returns a player's level: `"plv <fd> <level>\n"`. |
| `Pin` | `Pin.cpp` | Returns a player's position and full inventory: `"pin <fd> <x> <y> <food> <linemate> ...\n"`. |
| `Sgt` | `Sgt.cpp` | Documented as "reports the current time unit" but currently returns an empty string — not implemented. |
| `Sst` | `Sst.cpp` | Documented as "sets the time unit" but currently returns an empty string — not implemented. |

Note: `Sgt`/`Sst` are stubs (`(void)args; (void)client; return "";`); the time-unit query/set protocol commands are declared and wired into `CommandParser`'s `_graphicCommands` table but have no actual effect yet.
