#include "Network/Client/Client.hpp"
#include "Network/ClientHandler/Commands/Commands.hpp"
#include "Simulation/Utils.hpp"
#include "Simulation/World/World.hpp"
#include <criterion/criterion.h>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

struct PpoFixture
{
    int a, b;
    zappy::World world;
    std::queue<std::string> broadcastQueue;
    std::vector<std::unique_ptr<zappy::Client>> clients;
    zappy::Commands *commands;
    zappy::Client *client;
    int playerId;

    PpoFixture() : world(5, 5, 100)
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
    ~PpoFixture()
    {
        delete commands;
        delete client;
        close(b);
    }
};

Test(Ppo, returns_sbp_when_no_args_are_given)
{
    PpoFixture f;

    std::string res = f.commands->Ppo({}, *f.client);

    cr_assert_str_eq(res.c_str(), "sbp\n");
}

Test(Ppo, returns_ko_for_an_unknown_player_id)
{
    PpoFixture f;

    std::string res = f.commands->Ppo({"999"}, *f.client);

    cr_assert_str_eq(res.c_str(), "ko\n");
}

Test(Ppo, reports_position_and_default_north_orientation)
{
    PpoFixture f;

    std::string res = f.commands->Ppo({std::to_string(f.playerId)}, *f.client);

    std::string expected = "ppo " + std::to_string(f.playerId) + " 0 0 1\n";
    cr_assert_str_eq(res.c_str(), expected.c_str());
}

Test(Ppo, reflects_rotation_changes)
{
    PpoFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    player->setRotation(zappy::Degrees::EAST);

    std::string res = f.commands->Ppo({std::to_string(f.playerId)}, *f.client);

    std::string expected = "ppo " + std::to_string(f.playerId) + " 0 0 2\n";
    cr_assert_str_eq(res.c_str(), expected.c_str());
}

Test(Ppo, buildPpoMessage_matches_the_command_reply)
{
    PpoFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);

    std::string viaCommand = f.commands->Ppo({std::to_string(f.playerId)}, *f.client);
    std::string viaBuilder = f.commands->buildPpoMessage(*player);

    cr_assert_str_eq(viaCommand.c_str(), viaBuilder.c_str());
}

Test(Ppo, forward_broadcasts_an_updated_ppo_message)
{
    PpoFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);

    f.commands->Forward({}, *f.client, f.clients);

    // Forward pushes buildPipiMessage then buildPpoMessage, in that order.
    cr_assert_eq(f.broadcastQueue.size(), 2u);
    f.broadcastQueue.pop();
    std::string expected = f.commands->buildPpoMessage(*player);
    cr_assert_str_eq(f.broadcastQueue.front().c_str(), expected.c_str());
}

Test(Ppo, right_broadcasts_an_updated_ppo_message)
{
    PpoFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);

    f.commands->Right({}, *f.client, f.clients);

    cr_assert_eq(f.broadcastQueue.size(), 2u);
    f.broadcastQueue.pop();
    std::string expected = f.commands->buildPpoMessage(*player);
    cr_assert_str_eq(f.broadcastQueue.front().c_str(), expected.c_str());
}
