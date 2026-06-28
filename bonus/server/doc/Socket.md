# Socket

`src/server/Network/Socket/Socket.hpp` / `.cpp`

`Socket` is a thin RAII wrapper around a single BSD socket file descriptor. It exists so [`ClientHandler`](ClientHandler.md) never touches raw `socket()`/`bind()`/`listen()`/`accept()` calls directly, and so the listening descriptor is guaranteed to be closed when the wrapper is destroyed (no manual `close()` bookkeeping at the call site).

## Surface

| Method | Wraps | Notes |
|--------|-------|-------|
| `create(domain, type, protocol)` | `::socket()` | Stores the resulting fd in `_socket`; returns `false` on failure (`-1`). |
| `bind(port)` | `::bind()` | Builds a `sockaddr_in` with `AF_INET`, `INADDR_ANY` (listen on all interfaces), and `htons(port)`. |
| `listen()` | `::listen()` | Backlog is `SOMAXCONN` (the OS maximum), since the server has no reason to artificially cap the pending-connection queue. |
| `accept()` | `::accept()` | No client address is retrieved (`nullptr, nullptr`) — the server doesn't need the peer's address, only its fd. Returns `-1` on failure. |
| `getFd()` | — | Returns the owned descriptor (const and non-const overloads). |
| `setSocket(fd)` | — | Replaces the owned descriptor (used when adopting an already-open fd via the `Socket(int)` constructor). |

## Ownership

`_socket` defaults to `-1` (no descriptor). The destructor closes it only if it is not `-1`, so a default-constructed or moved-from `Socket` is safe to destroy. There is no copy protection at the `Socket` level (unlike [`Client`](Client.md)), but in practice only one `Socket` is ever created — the listening socket owned by `ClientHandler` — so the lack of an explicit copy-delete is not exercised.

## How ClientHandler uses it

`ClientHandler` holds exactly one `Socket _tcpSocket` member for the listening socket. Its constructor chains `create` → `bind` → `listen` to bring the server up; the event loop's `addClient()` calls `_tcpSocket.accept()` whenever `poll()` reports `POLLIN` on `_fds[0]` (the listening socket's fd, fetched once via `getFd()` at construction time). Each *accepted* connection becomes a [`Client`](Client.md), not a `Socket` — `Socket` is reserved for the one listening descriptor.
