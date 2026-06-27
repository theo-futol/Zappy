#include "Network/Client/Client.hpp"
#include "Network/ClientHandler/Commands/Commands.hpp"
#include "Simulation/Utils.hpp"
#include "Simulation/World/World.hpp"
#include <criterion/criterion.h>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

struct PipiFixture
{
    int a, b;
    zappy::World world;
    std::queue<std::string> broadcastQueue;
    std::vector<std::unique_ptr<zappy::Client>> clients;
    zappy::Commands *commands;
    zappy::Client *client;
    int playerId;

    PipiFixture() : world(5, 5, 100)
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
    ~PipiFixture()
    {
        delete commands;
        delete client;
        close(b);
    }
};

Test(Pipi, builds_a_snapshot_with_position_orientation_level_and_inventory)
{
    PipiFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);

    std::string msg = f.commands->buildPipiMessage(*player);

    std::string expected = "pipi " + std::to_string(f.playerId) + " 0 0 1 1 10 0 0 0 0 0 0 \n";
    cr_assert_str_eq(msg.c_str(), expected.c_str());
}

Test(Pipi, reflects_rotation_inventory_and_position_changes)
{
    PipiFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    player->setRotation(zappy::Degrees::SOUTH);
    player->getInventory().addItem(zappy::ItemType::DERAUMERE, 2);
    f.world.removePlayerFromTile(player, player->getPosition());
    player->setPosition(3, 4, f.world.getMapSize());
    f.world.addPlayerToTile(player, player->getPosition());

    std::string msg = f.commands->buildPipiMessage(*player);

    std::string expected = "pipi " + std::to_string(f.playerId) + " 3 4 3 1 10 0 2 0 0 0 0 \n";
    cr_assert_str_eq(msg.c_str(), expected.c_str());
}

Test(Pipi, forward_broadcasts_a_pipi_snapshot_before_the_ppo_update)
{
    PipiFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);

    f.commands->Forward({}, *f.client, f.clients);

    cr_assert_eq(f.broadcastQueue.size(), 2u);
    std::string expected = f.commands->buildPipiMessage(*player);
    cr_assert_str_eq(f.broadcastQueue.front().c_str(), expected.c_str());
}

Test(Pipi, eject_broadcasts_a_pipi_snapshot_for_the_ejected_player)
{
    PipiFixture f;
    int sv2[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv2);
    int ejectedId = f.world.addPlayer(sv2[0], "team1");
    zappy::Player *ejector = f.world.getPlayerById(f.playerId);
    zappy::Player *ejected = f.world.getPlayerById(ejectedId);
    f.world.removePlayerFromTile(ejected, ejected->getPosition());
    ejected->setPosition(ejector->getPosition().x, ejector->getPosition().y, f.world.getMapSize());
    f.world.addPlayerToTile(ejected, ejected->getPosition());

    f.commands->Eject({}, *f.client, f.clients);

    // Eject pushes pipi then ppo for the ejected player, then pex for the ejector.
    cr_assert_eq(f.broadcastQueue.size(), 3u);
    std::string expectedPipi = f.commands->buildPipiMessage(*ejected);
    cr_assert_str_eq(f.broadcastQueue.front().c_str(), expectedPipi.c_str());

    close(sv2[0]);
    close(sv2[1]);
}
