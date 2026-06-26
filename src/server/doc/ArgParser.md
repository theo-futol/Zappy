# ArgParser

`src/server/ArgParser/ArgParser.hpp` / `.cpp`

The `ArgParser` is a generic, runtime-configurable command-line argument parser. Unlike a hard-coded parser, it does not know about `-p`, `-x`, `-y`, etc. by itself: the caller registers the flags it expects before calling `parse()`. This keeps the parsing logic independent from the specific flags the Zappy server happens to need, and makes it trivial to add or remove a flag from `Core` without touching `ArgParser` itself.

## Data model

- `FlagType` — the shape of the value(s) a flag accepts:
  - `INT`: a single integer value (`-p 4242`)
  - `STRING`: a single string value (`-c foo`)
  - `LIST`: one or more string values, consumed until the next flag (`-n team1 team2`)
- `FlagConfig` — metadata attached to a registered flag: its `type`, whether it is `required`, and a human-readable `description` used in error messages.
- Internally, `ArgParser` keeps two maps:
  - `_flagConfigs`: flag token → `FlagConfig`, filled by `registerFlag()`.
  - `_parsedValues`: flag token → collected string values, filled by `parse()`.

## Lifecycle

1. **Construction** — `ArgParser(argc, argv)` only stores `argc`/`argv`. No parsing happens yet.
2. **Registration** — the caller calls `registerFlag(flag, type, required, description)` for every flag it wants to support. This must happen before `parse()`.
3. **Parsing** — `parse()` walks `argv` once:
   - Any token not starting with `-` outside of a flag's value position is rejected (`"Unexpected value without flag"`).
   - Any token starting with `-` that wasn't registered is rejected (`"Unknown flag"`).
   - For a `LIST` flag, every following token that doesn't start with `-` is appended to its value list, until the next flag or the end of argv. At least one value is required.
   - For `INT`/`STRING` flags, exactly the next token is consumed as the single value; missing it raises `"Flag ... requires a value"`.
   - After the scan, every registered flag with `required = true` that was never matched is reported, and `parse()` throws a single `ServerException` listing all missing flags at once (rather than failing on the first one), so the user sees the full requirement list in one error.
4. **Querying** — once parsed, values are retrieved through typed getters:
   - `hasFlag(flag)` — whether the flag was present.
   - `getInt(flag)` — parses the first collected value as an `int` (via `std::stoi`); throws if absent or not a valid integer.
   - `getString(flag)` — returns the first collected value.
   - `getList(flag)` — returns the full vector of values (used directly by `LIST` flags, but also the common path underneath `getInt`/`getString`).

All error paths throw `zappy::ServerException` (see [ServerException.md](ServerException.md)), so a malformed command line surfaces as a normal exception that `main()` catches and reports with exit code 84.

## Relationship with Core

`ArgParser` itself defines no Zappy-specific flags. `Core`'s constructor (see [Core.md](Core.md)) is the place where the actual server flags (`-p`, `-x`, `-y`, `-n`, `-c`, `-f`) are registered and range-validated. This separation means `ArgParser` could be reused for a different tool with a different flag set without modification.
