# Logger

`src/server/Logger/Logger.hpp` / `.cpp`

`Logger` is a single static utility used to emit structured log lines to `stdout`. It is intentionally minimal: one static method, no instances, no log levels, no destinations other than `stdout`. Its purpose is to give every part of the server a consistent line format for events worth tracing (player actions, world events, parsing/protocol issues) without coupling those callers to any particular logging framework.

## Format

Every line follows the protocol-flavored grammar:

```
<code> - <message> (<key>=<value>, <key>=<value>, ...)
```

- `code` — a short identifier (often numeric-looking, e.g. `"014"`, `"8424"`) used to group/filter log lines by event type. Codes are caller-defined; `Logger` does not validate or register them.
- `message` — a human-readable description, conventionally written as `"<Subject> : <what happened>"` (e.g. `"Player : created"`, `"Forward : failed"`).
- `fields` — an ordered list of `(key, value)` pairs (`Logger::Field = std::pair<std::string, std::string>`) rendered as `key=value`, comma-separated, inside parentheses. When `fields` is empty, the `(...)` group is omitted entirely rather than printed as `()`.

```cpp
Logger::log("014", "Forward : player moved",
            {{"player_id", std::to_string(player->getId())},
             {"x", std::to_string(newPos.x)},
             {"y", std::to_string(newPos.y)}});
// -> 014 - Forward : player moved (player_id=3, x=5, y=2)

Logger::log("020", "World : resource spawned on tile",
            {{"item", itemTypeToString(itemType)}, {"x", "1"}, {"y", "4"}});
// -> 020 - World : resource spawned on tile (item=food, x=1, y=4)
```

Values are always passed as `std::string`; numeric fields are converted with `std::to_string` at the call site — `Logger` itself does no formatting beyond string concatenation.

## Usage across the server

`Logger::log` is called directly wherever an event is worth tracing, with no intermediary object to construct or inject:

- **[Simulation](Simulation.md)** — `Player` logs creation and freezing (incantation rituals); `World` logs resource spawning, tile update failures, player death, and incantation failures.
- **[Commands](Commands.md)** — individual AI commands log their outcome, e.g. `Connect` (`Connect_nbr` response), `Move` (`Forward`/`Right`/`Left`), `Eject` (success/failure, egg destruction), `Inventory` (response sent).
- **[CommandParser](CommandParser.md)** — logs raw input reception and protocol-level rejections (frozen player attempting an action, dead player attempting an action).

Because `log()` is `static`, no `Logger` instance is created or threaded through constructors; any file that includes `Logger.hpp` can log immediately. This keeps the call sites terse at the cost of having no way to redirect output, filter by level, or disable logging at runtime — acceptable tradeoffs given the server only writes to `stdout` and logs are meant to mirror the protocol's own response codes for easy cross-referencing with the RFC (see `doc/RFC.md`).
