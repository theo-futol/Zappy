#include "Network/Client/Client.hpp"
#include "Simulation/World/World.hpp"
#include <criterion/criterion.h>
#include <queue>
#include <string>

Test(WorldBroadcast, removeIncantationStones_does_nothing_without_a_broadcast_queue)
{
    zappy::World world(5, 5, 100);
    world.setTileAt({0, 0}, zappy::ItemType::LINEMATE, 1);

    world.removeIncantationStones(0, 0, 1);

    int linemate = 0;
    for (const auto &item : world.getTileAt({0, 0})->_items)
        if (item.first == zappy::ItemType::LINEMATE)
            linemate = item.second;
    cr_assert_eq(linemate, 0);
}

Test(WorldBroadcast, removeIncantationStones_broadcasts_the_updated_tile_content)
{
    zappy::World world(5, 5, 100);
    std::queue<std::string> broadcastQueue;
    world.setBroadCastQueue(&broadcastQueue);
    world.setTileAt({0, 0}, zappy::ItemType::LINEMATE, 1);
    world.setTileAt({0, 0}, zappy::ItemType::FOOD, 0);

    world.removeIncantationStones(0, 0, 1);

    cr_assert_eq(broadcastQueue.size(), 1u);
    cr_assert_str_eq(broadcastQueue.front().c_str(), "bct 0 0 0 0 0 0 0 0 0\n");
}

Test(WorldBroadcast, removeIncantationStones_on_a_level_with_no_requirement_pushes_the_unchanged_tile)
{
    zappy::World world(5, 5, 100);
    std::queue<std::string> broadcastQueue;
    world.setBroadCastQueue(&broadcastQueue);

    world.removeIncantationStones(0, 0, 42);

    cr_assert_eq(broadcastQueue.size(), 0u);
}
