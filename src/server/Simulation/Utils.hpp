#pragma once

namespace zappy
{
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
