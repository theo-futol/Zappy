# Client

`src/server/Network/Client/Client.hpp` / `.cpp`, `ClientType.hpp`

A `Client` is the network-level representation of one accepted TCP connection: its file descriptor, its protocol role, and the bytes received from the socket that have not yet been consumed as a full command line. It knows nothing about the game itself — that belongs to [Player](Simulation.md) (AI clients) or to the broadcast queue (graphic clients). `Client` is purely "one socket, plus the partial line sitting in its receive buffer."

## Role: `ClientType`

```cpp
enum class ClientType { GRAPHIC, AI, DEAD, UNKNOWN };
```

Every accepted connection starts as `UNKNOWN`: the server does not yet know whether it is a GUI viewer or an AI player. The very first line sent by the client is interpreted as a handshake by [CommandParser](CommandParser.md)'s `_handleHandshake()` — `"GRAPHIC"` switches the client to `ClientType::GRAPHIC`, anything else is treated as a team name and switches it to `ClientType::AI` (rejecting the connection with `"ko\n"` if the team has no free slot).

`DEAD` is not a connection-level state but a marker set by [ClientHandler](ClientHandler.md) when the underlying `Player` has starved (`World::foodCheck()`); the next time the client's queued command is due, it receives a `"dead\n"` reply instead of being executed.

## Ownership and lifetime

A `Client` owns its file descriptor: the constructor wraps an already-`accept()`-ed fd, and the destructor `close()`s it. Copying is explicitly disabled (`= delete` on copy ctor/assignment) so a `Client` is never duplicated and the fd never double-closed; it always lives behind a `std::unique_ptr<Client>` in `ClientHandler::_clients`.

## The receive buffer

```cpp
std::string _buffer;
```

TCP is a byte stream, not a message stream: a single `recv()` call can return less than one full command line, or several lines, or part of a line. `Client` does not do the buffering logic itself — it just exposes the buffer for the owner to manipulate:

- `getBuffer()` (const and mutable overloads) — read or directly mutate the pending bytes.
- `setBuffer(buffer)` — replace the buffer outright.

[`CommandParser::feed()`](CommandParser.md) is the actual consumer: on each readable event it does `_client->getBuffer() + recv()`, splits on `'\n'` to extract complete lines (queuing each as a command), and writes back whatever incomplete tail remains via `setBuffer()`. This keeps the partial-line state attached to the `Client` itself rather than duplicated in the handler.

## Used by ClientHandler

[`ClientHandler`](ClientHandler.md) is the sole owner of `Client` instances: it creates one in `addClient()` right after `accept()`, looks them up by fd (`getClientByFd()`) to broadcast or to flag them `DEAD`, and destroys them in `removeClient()` once `poll()` reports a hangup or `CommandParser::feed()` reports the peer closed the connection.
