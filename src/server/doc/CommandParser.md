# CommandParser

`src/server/Network/ClientHandler/CommandParser/CommandParser.hpp` / `.cpp`

Each connected [`Client`](Client.md) gets its own `CommandParser`, constructed by [`ClientHandler::addClient()`](ClientHandler.md). It is the per-client pipeline that turns raw socket bytes into timed, validated calls into the shared [`Commands`](Commands.md) object, and turns the resulting reply string back into bytes on the wire. It also owns the lookup tables that decide which command names exist for AI clients versus graphic clients, and how long each AI command takes to execute in game time.

## Command tables

Built once per parser, in the constructor:

- `_initAICommands()` fills `_aiCommands`: name → `(duration, handler)`. Duration is expressed in *time units*, later scaled by `_f` (reciprocal of the time unit) into real milliseconds.

  | Command | Duration (time units) |
  |---|---|
  | `Forward`, `Right`, `Left`, `Look`, `Broadcast`, `Eject`, `Take`, `Set` | 7 |
  | `Inventory` | 1 |
  | `Connect_nbr` | 0 |
  | `Fork` | 42 |
  | `Incantation` | 300 |

- `_initGraphicCommands()` fills `_graphicCommands`: name → handler, with no duration — graphic commands are protocol/query commands (`msz`, `bct`, `mct`, `tna`, `ppo`, `plv`, `pin`, `sgt`, `sst`) and execute as soon as they are dequeued.

Both tables store `std::function<std::string(args, Client&, Commands&)>` lambdas that simply forward to the matching `Commands::<Name>()` method — see [Commands.md](Commands.md) for what each one actually does.

## Reading: `feed()`

Called by `ClientHandler` whenever `poll()` reports the client's fd is readable. `feed()`:

1. `recv()`s up to 4095 bytes. `bytes <= 0` means the peer disconnected or errored → returns `false` (the caller, `ClientHandler::clientEventHandling()`, then calls `removeClient`).
2. Prepends the client's leftover partial line (`Client::getBuffer()`) to the freshly received bytes.
3. Splits on `'\n'`, queuing one `PendingCommand` per complete line found, and discards empty lines.
4. For each line, looks up its command word in `_aiCommands` to get its duration cost (`0` if not found — i.e. unknown/graphic commands have no artificial delay at this stage); computes `readyAt` by stacking the duration on top of either "now" or the previous queued command's `readyAt`, whichever is later — so a client's own commands always execute strictly in order, one duration after another, regardless of how fast it sends them.
5. Special-cases `Incantation` for AI clients: rather than just queuing it, it calls `_commands.beginIncantation(*_client, readyAt)` immediately at parse time. If the player doesn't exist, is already frozen, or the ritual's prerequisites aren't met, the line is dropped entirely (`continue`) instead of being queued — this is what makes a failed `Incantation` attempt return `"ko\n"` right away rather than waiting out the 300-time-unit duration first.
6. Writes back whatever trailing partial line remains into `Client::setBuffer()`.

## Running: `executeNext()`

Called repeatedly by `ClientHandler::clientEventHandling()` (in a `while` loop) to drain every command whose time has come:

- Empty queue → `false` (nothing to do).
- More than 10 queued commands → the client is flagged `_isBanned = true` and `false` is returned; `ClientHandler` will then remove this client (anti-flood protection).
- `ClientType::DEAD` → the queued command is popped and replied to with `"dead\n"` without being dispatched at all; returns `true` so the caller keeps draining the rest of the (now-irrelevant) queue the same way.
- `ClientType::AI` and the matching `Player::isFrozen()` (mid-incantation) → `false`; the whole queue waits until the freeze lifts, even if some commands behind it are otherwise ready. This matches the protocol rule that a player taking part in an elevation ritual cannot act.
- Otherwise, if `now < front().readyAt`, `false` (not due yet).
- Else the line is popped and dispatched: `_handleHandshake()` if the client is still `UNKNOWN` (its very first line), `_dispatch()` otherwise.

## Handshake — `_handleHandshake()`

The first line ever sent by a freshly accepted client decides its [`ClientType`](Client.md):
- `"GRAPHIC"` → `ClientType::GRAPHIC`, no reply sent here (graphic clients drive everything else via query commands).
- anything else is treated as a **team name** → `ClientType::AI`. If the team has no available slot, replies `"ko\n"` and stops (the connection is left as `AI` type but never gets a `Player`, so subsequent commands will find no matching player and reply accordingly). On success it replies with `"<availableSlots>\n<width> <height>\n"` (the AI protocol's connection handshake), creates the `Player` (`World::addPlayer`), and pushes a `pnw` (player-new) event onto the broadcast queue so connected GUIs see the new player appear.

## Dispatch — `_dispatch()`

Tokenizes the line into a command word and a `std::vector<std::string>` of arguments, looks it up in `_aiCommands` or `_graphicCommands` depending on the client's type, calls the handler, and `send()`s the returned string back on the socket. Unknown AI commands reply `"ko\n"`; unknown graphic commands reply `"suc\n"` (the zappy GUI protocol's generic "command not recognized but connection still fine" acknowledgement) — note the different conventions for the two protocols.

## Why time matters here

`nextReadyAt()` exposes the front of `_commandQueue`'s `readyAt` (or `time_point::max()` if empty) so [`ClientHandler::handleClients()`](ClientHandler.md) can compute the right `poll()` timeout across *all* clients at once, instead of busy-waiting.
