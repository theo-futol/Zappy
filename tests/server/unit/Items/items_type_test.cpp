#include "Simulation/World/Items.hpp"
#include <criterion/criterion.h>
#include <string>

// ---- itemTypeToString -----------------------------------------------------

Test(ItemType, itemTypeToString_food)
{
    cr_assert_str_eq(zappy::itemTypeToString(zappy::ItemType::FOOD).c_str(), "food");
}

Test(ItemType, itemTypeToString_linemate)
{
    cr_assert_str_eq(zappy::itemTypeToString(zappy::ItemType::LINEMATE).c_str(), "linemate");
}

Test(ItemType, itemTypeToString_deraumere)
{
    cr_assert_str_eq(zappy::itemTypeToString(zappy::ItemType::DERAUMERE).c_str(), "deraumere");
}

Test(ItemType, itemTypeToString_sibur)
{
    cr_assert_str_eq(zappy::itemTypeToString(zappy::ItemType::SIBUR).c_str(), "sibur");
}

Test(ItemType, itemTypeToString_mendiane)
{
    cr_assert_str_eq(zappy::itemTypeToString(zappy::ItemType::MENDIANE).c_str(), "mendiane");
}

Test(ItemType, itemTypeToString_phiras)
{
    cr_assert_str_eq(zappy::itemTypeToString(zappy::ItemType::PHIRAS).c_str(), "phiras");
}

Test(ItemType, itemTypeToString_thystame)
{
    cr_assert_str_eq(zappy::itemTypeToString(zappy::ItemType::THYSTAME).c_str(), "thystame");
}

Test(ItemType, itemTypeToString_egg)
{
    cr_assert_str_eq(zappy::itemTypeToString(zappy::ItemType::EGG).c_str(), "egg");
}

Test(ItemType, itemTypeToString_unknown)
{
    cr_assert_str_eq(zappy::itemTypeToString(zappy::ItemType::UNKNOWN).c_str(), "unknown");
}

// ---- stringToItemType -----------------------------------------------------

Test(ItemType, stringToItemType_food)
{
    cr_assert_eq(zappy::stringToItemType("food"), zappy::ItemType::FOOD);
}

Test(ItemType, stringToItemType_linemate)
{
    cr_assert_eq(zappy::stringToItemType("linemate"), zappy::ItemType::LINEMATE);
}

Test(ItemType, stringToItemType_deraumere)
{
    cr_assert_eq(zappy::stringToItemType("deraumere"), zappy::ItemType::DERAUMERE);
}

Test(ItemType, stringToItemType_sibur)
{
    cr_assert_eq(zappy::stringToItemType("sibur"), zappy::ItemType::SIBUR);
}

Test(ItemType, stringToItemType_mendiane)
{
    cr_assert_eq(zappy::stringToItemType("mendiane"), zappy::ItemType::MENDIANE);
}

Test(ItemType, stringToItemType_phiras)
{
    cr_assert_eq(zappy::stringToItemType("phiras"), zappy::ItemType::PHIRAS);
}

Test(ItemType, stringToItemType_thystame)
{
    cr_assert_eq(zappy::stringToItemType("thystame"), zappy::ItemType::THYSTAME);
}

Test(ItemType, stringToItemType_egg)
{
    cr_assert_eq(zappy::stringToItemType("egg"), zappy::ItemType::EGG);
}

Test(ItemType, stringToItemType_unknown_string)
{
    cr_assert_eq(zappy::stringToItemType("notanitem"), zappy::ItemType::UNKNOWN);
    cr_assert_eq(zappy::stringToItemType(""), zappy::ItemType::UNKNOWN);
}
