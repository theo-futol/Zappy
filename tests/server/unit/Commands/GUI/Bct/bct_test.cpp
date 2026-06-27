#include "Network/Client/Client.hpp"
#include "Network/ClientHandler/Commands/Commands.hpp"
#include "Simulation/Utils.hpp"
#include "Simulation/World/World.hpp"
#include <criterion/criterion.h>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

struct BctFixture
{
    int a, b;
    zappy::World world;
    std::queue<std::string> broadcastQueue;
    std::vector<std::unique_ptr<zappy::Client>> clients;
    zappy::Commands *commands;
    zappy::Client *client;
    int playerId;

    BctFixture() : world(5, 5, 100)
    {
        int sv[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
        a = sv[0];
        b = sv[1];
        client = new zappy::Client(a);
        world.addTeam("team1", 0, 5);
        playerId = world.addPlayer(a, "team1");
        client->setPlayerId(playerId);
        // Eggs hatch at a random tile; pin the spawn to (0,0) so position assertions
        // are deterministic.
        zappy::Player *spawned = world.getPlayerById(playerId);
        world.removePlayerFromTile(spawned, spawned->getPosition());
        spawned->setPosition(0, 0, world.getMapSize());
        world.addPlayerToTile(spawned, spawned->getPosition());
        commands = new zappy::Commands(&world, &broadcastQueue);
    }
    ~BctFixture()
    {
        delete commands;
        delete client;
        close(b);
    }
};

Test(Bct, returns_ko_when_fewer_than_two_args_are_given)
{
    BctFixture f;

    std::string res = f.commands->Bct({"bct", "0"}, *f.client);

    cr_assert_str_eq(res.c_str(), "ko\n");
}

Test(Bct, returns_ko_for_a_tile_outside_the_map)
{
    BctFixture f;

    std::string res = f.commands->Bct({"bct", "99", "99"}, *f.client);

    cr_assert_str_eq(res.c_str(), "ko\n");
}

Test(Bct, reports_resource_counts_for_a_tile)
{
    BctFixture f;
    f.world.setTileAt({1, 1}, zappy::ItemType::FOOD, 2);
    f.world.setTileAt({1, 1}, zappy::ItemType::LINEMATE, 3);
    f.world.setTileAt({1, 1}, zappy::ItemType::DERAUMERE, 0);
    f.world.setTileAt({1, 1}, zappy::ItemType::SIBUR, 0);
    f.world.setTileAt({1, 1}, zappy::ItemType::MENDIANE, 0);
    f.world.setTileAt({1, 1}, zappy::ItemType::PHIRAS, 0);
    f.world.setTileAt({1, 1}, zappy::ItemType::THYSTAME, 0);

    std::string res = f.commands->Bct({"bct", "1", "1"}, *f.client);

    cr_assert_str_eq(res.c_str(), "bct 1 1 2 3 0 0 0 0 0\n");
}

Test(Bct, buildBctMessage_matches_the_command_reply)
{
    BctFixture f;

    std::string viaCommand = f.commands->Bct({"bct", "0", "0"}, *f.client);
    std::string viaBuilder = f.commands->buildBctMessage({0, 0});

    cr_assert_str_eq(viaCommand.c_str(), viaBuilder.c_str());
}

Test(Bct, take_broadcasts_an_updated_bct_message_for_the_tile)
{
    BctFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    f.world.setTileAt(player->getPosition(), zappy::ItemType::LINEMATE, 2);

    f.commands->Take({"linemate"}, *f.client, f.clients);

    // Take pushes pgt, pipi, pin, bct, in that order.
    cr_assert_eq(f.broadcastQueue.size(), 4u);
    std::string expected = f.commands->buildBctMessage(player->getPosition());
    cr_assert_str_eq(f.broadcastQueue.back().c_str(), expected.c_str());
}

Test(Bct, set_broadcasts_an_updated_bct_message_for_the_tile)
{
    BctFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    f.world.setTileAt(player->getPosition(), zappy::ItemType::LINEMATE, 0);
    player->getInventory().addItem(zappy::ItemType::LINEMATE, 2);

    f.commands->Set({"linemate"}, *f.client, f.clients);

    // Set pushes pdr, pipi, pin, bct, in that order.
    cr_assert_eq(f.broadcastQueue.size(), 4u);
    std::string expected = f.commands->buildBctMessage(player->getPosition());
    cr_assert_str_eq(f.broadcastQueue.back().c_str(), expected.c_str());
}
