#pragma once

namespace zappy
{
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
