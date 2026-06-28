# ServerException

`src/server/ServerException/ServerException.hpp` / `.cpp`

`ServerException` is the single exception type used throughout the server — [`ArgParser`](ArgParser.md), [`Core`](Core.md), [`Socket`](Socket.md)/[`ClientHandler`](ClientHandler.md) (e.g. a failing `poll()`), and the [Simulation](Simulation.md) layer (e.g. an invalid rotation in `Look`) all throw it rather than defining their own exception hierarchy.

It derives from `std::exception` and stores two things at construction time:

- `_message` — the human-readable description passed by the caller.
- `_location` — a `std::source_location`, captured automatically via a defaulted `std::source_location::current()` parameter, so the throw site does not need to pass it explicitly.

`what()` lazily formats both into a single string: `"<message> (at <file>:<line>:<column>)"`. This means every uncaught `ServerException` printed by `main()`'s top-level `catch` block self-reports exactly where it was thrown, without any logging infrastructure.

```cpp
throw ServerException("Unknown flag: " + token);
// -> "Unknown flag: -z (at src/server/ArgParser/ArgParser.cpp:23:9)"
```

`main.cpp` catches `zappy::ServerException` exactly once, prints `what()` to `stderr`, and exits with code `84`.
