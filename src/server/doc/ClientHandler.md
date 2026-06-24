# ClientHandler

`src/server/Network/ClientHandler/ClientHandler.hpp` / `.cpp`

`ClientHandler` owns the listening [`Socket`](Socket.md), every connected [`Client`](Client.md), one [`CommandParser`](CommandParser.md) per client, and the single blocking event loop that drives the whole server once `Core::run()` is called. It is the piece that turns "network bytes available" and "simulation tick elapsed" into concrete actions: accepting connections, feeding/executing commands, regenerating world resources, starving players, checking the win condition, and flushing both kinds of outgoing traffic (GUI broadcast queue, per-player delayed messages).

## Construction

```cpp
ClientHandler(int port, int initialClientCapacity, int f, World *world, bool *serverIsRunning);
```

The constructor creates, binds and listens the TCP socket (`AF_INET`, `SOCK_STREAM`), reserves `initialClientCapacity + 1` slots in the `pollfd` vector, and pushes the listening socket as `_fds[0]` — by convention, index 0 is always the listening socket and every index `>= 1` is a connected client. `f` (reciprocal of the time unit) and `world`/`serverIsRunning` are stored as-is; they come from [Core](Core.md).

## The event loop — `handleClients()`

The loop runs `while (*_serverIsRunning)`. Each iteration:

1. **Compute the next deadline.** Three kinds of periodic work can be due: world resource regeneration (every `RESOURCE_INTERVAL_TIME_UNITS = 20` time units), food consumption/starvation (every `CYCLE_TO_DIE = 126` time units), and each client's next queued command (`CommandParser::nextReadyAt()`). The loop takes the *earliest* of all these deadlines and passes the remaining milliseconds as `poll()`'s timeout — so `poll()` returns either when I/O is ready or exactly when the next scheduled tick is due, never later.
2. **`poll(_fds.data(), _fds.size(), timeout)`.** `EINTR` is retried; any other failure throws a `ServerException`. This is the only place the loop can block.
3. **Accept.** If `_fds[0].revents & POLLIN`, a new connection is pending → `addClient()`.
4. **Resource tick.** If the resource interval elapsed, `_world->ressourcePassiveGeneration()` runs and `_lastResourceUpdate` resets.
5. **Food tick.** If the food interval elapsed, `_world->foodCheck()` consumes one food unit per player and returns the fds of players that starved; those clients are marked `ClientType::DEAD` (their next due command will get a `"dead\n"` reply instead of executing — see [CommandParser](CommandParser.md)).
6. **`clientEventHandling()`** — see below: reads from clients and runs due commands.
7. **Win condition.** `_world->checkWinningCondition()` — if a team has won, `*_serverIsRunning` is flipped to `false` and the loop exits after this iteration's broadcasts.
8. **`broadcastGuiInfo()`** — flushes `_broadcastQueue` (world/game events such as `pnw`, `pbc`, `pdi`, `pic`, `pie`, `seg`, …) to every `GRAPHIC` client.
9. **`broadcastMessageToClients()`** — despite its name, this delivers each AI player's *own* delayed message queue (`Player::sendMessageToClient()`), not a broadcast to GRAPHIC clients; it is the mechanism that turns a `Broadcast` command into delayed `"message N, text\n"` deliveries to other AI players once their simulated travel time has elapsed. (Note: the header comment on this method is misleading — it says "Broadcasts a message to all clients of type GRAPHIC", but the implementation iterates `ClientType::AI` clients and calls `Player::sendMessageToClient()`.)

## `clientEventHandling()`

For every connected client fd (`_fds[1..]`):
- `POLLHUP` → `removeClient(fd)` immediately (the client closed/reset the connection).
- `POLLIN` → look up its `CommandParser` and call `feed()`; if `feed()` returns `false` (peer closed mid-`recv`), `removeClient(fd)`.

Once all sockets have been drained, every parser gets a chance to run its due commands: `while (parser->executeNext());` keeps draining a single client's ready queue until `executeNext()` says nothing more is ready (queue empty, next command not yet due, or the player is frozen mid-incantation). Parsers that have been `isBanned()` (flooded the server with more than 10 queued commands) are collected and their clients removed *after* this loop, to avoid mutating `_parsers` while iterating it.

## Connecting and disconnecting

- **`addClient()`** — `accept()`s the pending connection, immediately sends `"WELCOME\n"` (the zappy protocol's handshake banner), then creates the `Client` and its dedicated `CommandParser`, and appends a `pollfd` entry for it.
- **`removeClient(fd)`** — the single cleanup path used for every disconnect reason (hangup, read error, ban, or starvation-triggered player death is handled separately via `World::removePlayer` inside `foodCheck`/elsewhere). It tells the `World` to drop the player (`_world->removePlayer(fd)`, freeing the team slot and removing it from its tile), erases the `CommandParser`, the `Client`, and the `pollfd` entry, in that order.

## Broadcast queue

`ClientHandler` owns `std::queue<std::string> _broadcastQueue`, exposed via `getBroadcastQueue()`. [`Core::setWorldBroadcast()`](Core.md) wires this same queue into `World` (`setBroadCastQueue`) and into every `CommandParser`/`Commands` instance it constructs, so any part of the simulation or command logic can push a line destined for every `GRAPHIC` client; `broadcastGuiInfo()` is the single place that actually drains it onto the wire.

## Relationship to Core and World

`ClientHandler` does not own the `World` — it holds a non-owning `World *` supplied by [Core](Core.md) at construction time, and the `bool *serverIsRunning` pointing at `Core::serverIsRunning`, which is how `Core`'s signal handler (`SIGINT`/`SIGTERM`) cooperatively stops `handleClients()`'s loop.
