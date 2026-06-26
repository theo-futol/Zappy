# Utils

`src/server/Simulation/Utils.hpp`

A small header of shared types and helpers used across the [Simulation](Simulation.md) layer (`Player`, `Team`) — positions, compass directions, and player lifecycle state. It has no `.cpp`; everything is header-only (`inline`/`static`/`struct` definitions).

## `position`

```cpp
struct position { int x; int y; };
```

A plain tile coordinate. `operator==` is defined so positions can be compared directly (used by `World::getPlayersOnTileAtLevel`, `Team::hasEggAtPosition`, etc.). There is no built-in wrap-around on `position` itself — every consumer (`Player::setPosition`, `Player::nextPosition`, `Player::getDirectionTo`/`getDistanceTo`) re-implements the toroidal wrap against a given `mapSize` at the point of use, since `position` alone doesn't know the map's dimensions.

## `Degrees`

```cpp
enum Degrees { NORTH = 0, NORTH_EAST = 45, EAST = 90, EAST_SOUTH = 135,
               SOUTH = 180, SOUTH_WEST = 225, WEST = 270 };
```

The eight compass directions used for player facing and for the relative direction reported by `Broadcast`/`Look`. Despite the name including diagonals, the protocol only ever rotates a player in 90° steps (`Right`/`Left`); the 45° values exist so `getDirectionTo()` can express *relative bearing to another player/tile* more precisely than the four cardinal directions, matching the zappy protocol's 1–8 broadcast-direction sectors (`getDirectionTo(...) / 45 + 1`).

## `Direction::getNearestDirection(value)`

Given an arbitrary bearing in degrees (0–360, e.g. from `atan2`), snaps it to the nearest one of the eight `Degrees` values (including a duplicate `NORTH` entry at `360` so bearings just under `360°` still round correctly to `NORTH` rather than off the end of the table). Implemented with `std::min_element` over a fixed lookup table of `(Degrees, value)` pairs, comparing absolute differences. Used exclusively by `Player::getDirectionTo()`.

## `PlayerState`

```cpp
enum class PlayerState { PENDING, ALIVE, DEAD };
```

Player lifecycle marker (see [Simulation.md](Simulation.md#player) for how it's actually used — note that nothing in the current codebase transitions a player from `PENDING` to `ALIVE`).
