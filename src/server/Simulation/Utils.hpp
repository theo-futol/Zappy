#pragma once

namespace zappy
{

/// @brief Compass bearing in degrees, used for player orientation and direction maths.
enum Degrees
{
    NORTH = 0,
    NORTH_EAST = 45,
    EAST = 90,
    EAST_SOUTH = 135,
    SOUTH = 180,
    SOUTH_WEST = 225,
    WEST = 270
};

/// @brief Lifecycle of a player slot: waiting to hatch, active in the world, or dead.
enum class PlayerState
{
    PENDING,
    ALIVE,
    DEAD
};

/// @brief A tile coordinate on the map.
struct position
{
    int x;
    int y;
};
inline bool operator==(const position &lhs, const position &rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

} // namespace zappy
