#pragma once

#include <algorithm>
#include <array>
#include <cstdlib>

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
    WEST = 270,
    NORTH_WEST = 315,
    NONE = -1
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
                                                                 {Degrees::NORTH_WEST, 315}}};
        auto angularDistance = [](int a, int b) {
            int diff = std::abs(a - b);
            return std::min(diff, 360 - diff);
        };

        return std::min_element(directions.begin(), directions.end(),
                                [value, angularDistance](const Direction &a, const Direction &b) {
                                    int diffA = angularDistance(a.value, value);
                                    int diffB = angularDistance(b.value, value);
                                    return diffA < diffB;
                                })
            ->degree;
    }
    static int getDirectionValue(Degrees degree)
    {
        switch (degree)
        {
            case Degrees::NORTH_EAST:
            return 8;
        case Degrees::EAST:
            return 7;
        case Degrees::EAST_SOUTH:
            return 6;
        case Degrees::SOUTH:
            return 5;
        case Degrees::SOUTH_WEST:
            return 4;
        case Degrees::WEST:
        return 3;
        case Degrees::NORTH_WEST:
            return 2;
        case Degrees::NORTH:
            return 1;            
        default:
            return 0; // Default to NORTH if invalid
        }
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
