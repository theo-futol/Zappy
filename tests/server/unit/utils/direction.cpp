#include "Simulation/Utils.hpp"
#include <criterion/criterion.h>

// ---- getDirectionValue ---------------------------------------------------

Test(Direction, getDirectionValue_north_is_1)
{
    cr_assert_eq(zappy::Direction::getDirectionValue(zappy::Degrees::NORTH), 1);
}

Test(Direction, getDirectionValue_north_west_is_2)
{
    cr_assert_eq(zappy::Direction::getDirectionValue(zappy::Degrees::NORTH_WEST), 2);
}

Test(Direction, getDirectionValue_west_is_3)
{
    cr_assert_eq(zappy::Direction::getDirectionValue(zappy::Degrees::WEST), 3);
}

Test(Direction, getDirectionValue_south_west_is_4)
{
    cr_assert_eq(zappy::Direction::getDirectionValue(zappy::Degrees::SOUTH_WEST), 4);
}

Test(Direction, getDirectionValue_south_is_5)
{
    cr_assert_eq(zappy::Direction::getDirectionValue(zappy::Degrees::SOUTH), 5);
}

Test(Direction, getDirectionValue_east_south_is_6)
{
    cr_assert_eq(zappy::Direction::getDirectionValue(zappy::Degrees::EAST_SOUTH), 6);
}

Test(Direction, getDirectionValue_east_is_7)
{
    cr_assert_eq(zappy::Direction::getDirectionValue(zappy::Degrees::EAST), 7);
}

Test(Direction, getDirectionValue_north_east_is_8)
{
    cr_assert_eq(zappy::Direction::getDirectionValue(zappy::Degrees::NORTH_EAST), 8);
}

Test(Direction, getDirectionValue_none_returns_0)
{
    cr_assert_eq(zappy::Direction::getDirectionValue(zappy::Degrees::NONE), 0);
}

// ---- getNearestDirection -------------------------------------------------

Test(Direction, getNearestDirection_0_is_north)
{
    cr_assert_eq(zappy::Direction::getNearestDirection(0), zappy::Degrees::NORTH);
}

Test(Direction, getNearestDirection_360_is_north)
{
    // 360 wraps back to NORTH (angular distance of 0 on both sides)
    cr_assert_eq(zappy::Direction::getNearestDirection(360), zappy::Degrees::NORTH);
}

Test(Direction, getNearestDirection_90_is_east)
{
    cr_assert_eq(zappy::Direction::getNearestDirection(90), zappy::Degrees::EAST);
}

Test(Direction, getNearestDirection_180_is_south)
{
    cr_assert_eq(zappy::Direction::getNearestDirection(180), zappy::Degrees::SOUTH);
}

Test(Direction, getNearestDirection_270_is_west)
{
    cr_assert_eq(zappy::Direction::getNearestDirection(270), zappy::Degrees::WEST);
}

Test(Direction, getNearestDirection_45_is_north_east)
{
    cr_assert_eq(zappy::Direction::getNearestDirection(45), zappy::Degrees::NORTH_EAST);
}

Test(Direction, getNearestDirection_135_is_east_south)
{
    cr_assert_eq(zappy::Direction::getNearestDirection(135), zappy::Degrees::EAST_SOUTH);
}

Test(Direction, getNearestDirection_225_is_south_west)
{
    cr_assert_eq(zappy::Direction::getNearestDirection(225), zappy::Degrees::SOUTH_WEST);
}

Test(Direction, getNearestDirection_315_is_north_west)
{
    cr_assert_eq(zappy::Direction::getNearestDirection(315), zappy::Degrees::NORTH_WEST);
}

Test(Direction, getNearestDirection_60_snaps_to_east_south_or_east)
{
    // 60 is closer to 45 (NE, d=15) than 90 (E, d=30) — actually NE
    zappy::Degrees d = zappy::Direction::getNearestDirection(60);
    cr_assert_eq(d, zappy::Degrees::NORTH_EAST);
}

Test(Direction, getNearestDirection_100_snaps_to_east)
{
    // 100: dist to EAST(90)=10, dist to EAST_SOUTH(135)=35 → EAST
    cr_assert_eq(zappy::Direction::getNearestDirection(100), zappy::Degrees::EAST);
}

// ---- position equality ---------------------------------------------------

Test(Position, equality_same_coords)
{
    zappy::position p1{3, 7};
    zappy::position p2{3, 7};
    cr_assert(p1 == p2);
}

Test(Position, equality_different_x)
{
    zappy::position p1{1, 7};
    zappy::position p2{3, 7};
    cr_assert_not(p1 == p2);
}

Test(Position, equality_different_y)
{
    zappy::position p1{3, 5};
    zappy::position p2{3, 7};
    cr_assert_not(p1 == p2);
}
