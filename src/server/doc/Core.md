# Core

`src/server/Core/Core.hpp` / `.cpp`

`Core` is the top-level orchestrator of the server. It owns the two big subsystems — the [Simulation](Simulation.md) (`World`) and the network layer ([ClientHandler](ClientHandler.md)) — and is responsible for turning parsed command-line arguments into a running server, plus handling process-level signals for graceful shutdown.

## Construction and argument validation

`Core(ArgParser argParser)` takes ownership of an already-constructed but not-yet-parsed [`ArgParser`](ArgParser.md) (moved into `_argParser`). In its constructor it:

1. Installs the signal handler (`setSignalHandler()`).
2. Registers all the flags the Zappy server understands:

   | Flag | Type   | Required | Meaning |
   |------|--------|----------|---------|
   | `-p` | INT    | yes | TCP port to listen on |
   | `-x` | INT    | yes | width of the world |
   | `-y` | INT    | yes | height of the world |
   | `-n` | LIST   | yes | team names |
   | `-c` | INT    | yes | number of initial client slots per team |
   | `-f` | INT    | no  | reciprocal of the time unit (ticks per second), defaults to `100` |

3. Calls `_argParser.parse()`.
4. Performs range validation that goes beyond what `ArgParser` itself can express (since `ArgParser` only knows about flag *shape*, not domain constraints):
   - port must be `<= 65535`
   - world width/height must be in `[1, MAX_MAP_SIZE]` (`MAX_MAP_SIZE = 1024`)
   - client count per team must be `<= MAX_CLIENTS` (`256`)
   - number of teams must be in `[1, MAX_TEAMS]` (`4`)
   - if `-f` is present, it must be in `[1, MAX_F]` (`1000`)

   Any violation throws a `ServerException`, caught by `main()` which prints the error and exits with code 84.
5. Resets `serverIsRunning` to `true`.

At this point nothing has been allocated yet — `_clientHandler` and `_world` are both `nullptr`.

## Building the world and the network

`Core` exposes two explicit setup steps that `main()` calls in order:

- **`setWorld()`** — reads `-x`, `-y`, `-c`, `-n` and constructs the `World` (`std::make_unique<World>(width, height)`), then registers each team name found in `-n` with `World::addTeam(name, index, initialSlots)`.
- **`setClientHandler()`** — reads `-p`, `-f` (or its default `100`) and computes `initialClientCapacity = (-c) * (number of teams)`, then constructs the `ClientHandler` with a pointer to the already-built `World` and to the static `serverIsRunning` flag. Right after constructing it, `setWorldBroadcast()` wires the world's broadcast queue to the client handler's broadcast queue (`World::setBroadCastQueue`), so simulation events (deaths, eggs hatching, etc.) can flow out to connected GUI/AI clients.

The order matters: `setWorld()` must run before `setClientHandler()`, because the `ClientHandler` constructor takes a raw `World*` and `setWorldBroadcast()` dereferences `_world`. `main.cpp` follows this order explicitly.

## Signal handling

`Core::serverIsRunning` is a `static bool`, shared across the whole process. `setSignalHandler()` installs `Core::signalHandler` for both `SIGINT` and `SIGTERM` via `std::signal`. The handler itself only flips `serverIsRunning` to `false` — it performs no other work, which is the safe minimum for a POSIX signal handler. The actual shutdown happens cooperatively: the [ClientHandler](ClientHandler.md)'s `poll()`-based event loop checks `serverIsRunning` on each iteration and exits cleanly when it becomes `false`.

## Running

`run()` is a thin call into `_clientHandler->handleClients()`, which blocks for the lifetime of the server (see [ClientHandler.md](ClientHandler.md) for the event loop itself).

## main()

`main.cpp` ties everything together:

```cpp
zappy::ArgParser argParser(ac, av);
zappy::Core core(argParser);
core.setWorld();
core.setClientHandler();
core.run();
```

Any `zappy::ServerException` thrown anywhere in this chain (bad arguments, socket failure, etc.) is caught at this single top-level point, printed to `stderr`, and turned into exit code `84`.
