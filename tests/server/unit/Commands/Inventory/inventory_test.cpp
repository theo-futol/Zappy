#include "Network/Client/Client.hpp"
#include "Network/ClientHandler/Commands/Commands.hpp"
#include "Simulation/Utils.hpp"
#include "Simulation/World/World.hpp"
#include <criterion/criterion.h>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

struct InventoryFixture
{
    int a, b;
    zappy::World world;
    std::queue<std::string> broadcastQueue;
    std::vector<std::unique_ptr<zappy::Client>> clients;
    zappy::Commands *commands;
    zappy::Client *client;
    int playerId;

    InventoryFixture() : world(5, 5, 100)
    {
        int sv[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
        a = sv[0];
        b = sv[1];
        client = new zappy::Client(a);
        world.addTeam("team1", 0, 5);
        playerId = world.addPlayer(a, "team1");
        client->setPlayerId(playerId);
        zappy::Player *spawned = world.getPlayerById(playerId);
        world.removePlayerFromTile(spawned, spawned->getPosition());
        spawned->setPosition(0, 0, world.getMapSize());
        world.addPlayerToTile(spawned, spawned->getPosition());
        commands = new zappy::Commands(&world, &broadcastQueue);
    }
    ~InventoryFixture()
    {
        delete commands;
        delete client;
        close(b);
    }
};

Test(Inventory, returns_ko_when_caller_has_no_player)
{
    InventoryFixture f;
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    zappy::Client ghost(sv[0]);

    std::string res = f.commands->getInventory({}, ghost, f.clients);

    cr_assert_str_eq(res.c_str(), "ko\n");
    close(sv[0]);
    close(sv[1]);
}

Test(Inventory, returns_the_formatted_default_inventory)
{
    InventoryFixture f;

    std::string res = f.commands->getInventory({}, *f.client, f.clients);

    // Every new player starts with 10 food and nothing else.
    cr_assert_str_eq(res.c_str(), "[food 10, linemate 0, deraumere 0, sibur 0, mendiane 0, phiras 0, thystame 0]\n");
}

Test(Inventory, reflects_items_added_to_the_inventory)
{
    InventoryFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    player->getInventory().addItem(zappy::ItemType::LINEMATE, 3);

    std::string res = f.commands->getInventory({}, *f.client, f.clients);

    cr_assert_str_eq(res.c_str(), "[food 10, linemate 3, deraumere 0, sibur 0, mendiane 0, phiras 0, thystame 0]\n");
}

Test(Inventory, ignores_unused_args)
{
    InventoryFixture f;

    std::string res = f.commands->getInventory({"ignored"}, *f.client, f.clients);

    cr_assert_str_eq(res.c_str(), "[food 10, linemate 0, deraumere 0, sibur 0, mendiane 0, phiras 0, thystame 0]\n");
}
