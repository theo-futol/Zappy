#pragma once

namespace zappy
{

enum Corner
{
    NORTH = 0,
    NORTH_EAST = 45,
    EAST = 90,
    EAST_SOUTH = 135,
    SOUTH = 180,
    SOUTH_WEST = 225,
    WEST = 270
};

enum class PlayerState
{
    PENDING,
    ALIVE,
    DEAD
};

struct position
{
    size_t x;
    size_t y;
};
inline bool operator==(const position &lhs, const position &rhs)
{
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

} // namespace zappy
