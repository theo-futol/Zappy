#include "Network/Client/Client.hpp"
#include "Network/ClientHandler/Commands/Commands.hpp"
#include "Simulation/Utils.hpp"
#include "Simulation/World/World.hpp"
#include <criterion/criterion.h>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

struct PinFixture
{
    int a, b;
    zappy::World world;
    std::queue<std::string> broadcastQueue;
    std::vector<std::unique_ptr<zappy::Client>> clients;
    zappy::Commands *commands;
    zappy::Client *client;
    int playerId;

    PinFixture() : world(5, 5, 100)
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
    ~PinFixture()
    {
        delete commands;
        delete client;
        close(b);
    }
};

Test(Pin, returns_ko_when_no_args_are_given)
{
    PinFixture f;

    std::string res = f.commands->Pin({}, *f.client);

    cr_assert_str_eq(res.c_str(), "ko\n");
}

Test(Pin, returns_ko_for_an_unknown_player_id)
{
    PinFixture f;

    std::string res = f.commands->Pin({"pin", "999"}, *f.client);

    cr_assert_str_eq(res.c_str(), "ko\n");
}

Test(Pin, reports_position_and_the_default_inventory)
{
    PinFixture f;

    std::string res = f.commands->Pin({"pin", std::to_string(f.playerId)}, *f.client);

    std::string expected = "pin " + std::to_string(f.playerId) + " 0 0 10 0 0 0 0 0 0 \n";
    cr_assert_str_eq(res.c_str(), expected.c_str());
}

Test(Pin, reflects_inventory_changes)
{
    PinFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    player->getInventory().addItem(zappy::ItemType::LINEMATE, 3);

    std::string res = f.commands->Pin({"pin", std::to_string(f.playerId)}, *f.client);

    std::string expected = "pin " + std::to_string(f.playerId) + " 0 0 10 3 0 0 0 0 0 \n";
    cr_assert_str_eq(res.c_str(), expected.c_str());
}

Test(Pin, buildPinMessage_matches_the_command_reply)
{
    PinFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);

    std::string viaCommand = f.commands->Pin({"pin", std::to_string(f.playerId)}, *f.client);
    std::string viaBuilder = f.commands->buildPinMessage(*player);

    cr_assert_str_eq(viaCommand.c_str(), viaBuilder.c_str());
}

Test(Pin, take_broadcasts_an_updated_pin_message)
{
    PinFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    f.world.setTileAt(player->getPosition(), zappy::ItemType::LINEMATE, 2);

    f.commands->Take({"linemate"}, *f.client, f.clients);

    // Take pushes pgt, pipi, pin, bct, in that order.
    cr_assert_eq(f.broadcastQueue.size(), 4u);
    f.broadcastQueue.pop();
    f.broadcastQueue.pop();
    std::string expected = f.commands->buildPinMessage(*player);
    cr_assert_str_eq(f.broadcastQueue.front().c_str(), expected.c_str());
}

Test(Pin, set_broadcasts_an_updated_pin_message)
{
    PinFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    f.world.setTileAt(player->getPosition(), zappy::ItemType::LINEMATE, 0);
    player->getInventory().addItem(zappy::ItemType::LINEMATE, 2);

    f.commands->Set({"linemate"}, *f.client, f.clients);

    // Set pushes pdr, pipi, pin, bct, in that order.
    cr_assert_eq(f.broadcastQueue.size(), 4u);
    f.broadcastQueue.pop();
    f.broadcastQueue.pop();
    std::string expected = f.commands->buildPinMessage(*player);
    cr_assert_str_eq(f.broadcastQueue.front().c_str(), expected.c_str());
}
