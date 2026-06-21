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

struct Direction
{
    Degrees degree;
    int value;
    static Degrees getNearestDirection(int value)
    {
        static constexpr std::array<Direction, 8> directions = {{{Degrees::NORTH, 0},
                                                                 {Degrees::NORTH_EAST, 45},
                                                                 {Degrees::EAST, 90},
                                                                 {Degrees::EAST_SOUTH, 135},
                                                                 {Degrees::SOUTH, 180},
                                                                 {Degrees::SOUTH_WEST, 225},
                                                                 {Degrees::WEST, 270},
                                                                 {Degrees::NORTH, 360}}};

        return std::min_element(directions.begin(), directions.end(),
                                [value](const Direction &a, const Direction &b) {
                                    int diffA = std::abs(a.value - value);
                                    int diffB = std::abs(b.value - value);
                                    return diffA < diffB;
                                })
            ->degree;
    }
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
