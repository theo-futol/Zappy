#include "Network/Client/Client.hpp"
#include "Network/ClientHandler/Commands/Commands.hpp"
#include "Simulation/Utils.hpp"
#include "Simulation/World/World.hpp"
#include <criterion/criterion.h>
#include <fcntl.h>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

struct PlvFixture
{
    int a, b;
    zappy::World world;
    std::queue<std::string> broadcastQueue;
    std::vector<std::unique_ptr<zappy::Client>> clients;
    zappy::Commands *commands;
    zappy::Client *client;
    int playerId;

    PlvFixture() : world(5, 5, 100)
    {
        int sv[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
        a = sv[0];
        b = sv[1];
        int flags = fcntl(b, F_GETFL, 0);
        fcntl(b, F_SETFL, flags | O_NONBLOCK);
        clients.push_back(std::make_unique<zappy::Client>(a));
        client = clients[0].get();
        world.addTeam("team1", 0, 5);
        playerId = world.addPlayer(a, "team1");
        client->setPlayerId(playerId);
        // Eggs hatch at a random tile; pin the spawn to (0,0) so position assertions
        // are deterministic, and clear any leftover egg that randomly landed there too.
        zappy::Player *spawned = world.getPlayerById(playerId);
        world.removePlayerFromTile(spawned, spawned->getPosition());
        spawned->setPosition(0, 0, world.getMapSize());
        world.addPlayerToTile(spawned, spawned->getPosition());
        spawned->getTeam().removeEgg(spawned->getPosition(), -1);
        commands = new zappy::Commands(&world, &broadcastQueue);
    }
    ~PlvFixture()
    {
        delete commands;
        close(b);
    }
};

Test(Plv, returns_sbp_when_no_args_are_given)
{
    PlvFixture f;

    std::string res = f.commands->Plv({}, *f.client);

    cr_assert_str_eq(res.c_str(), "sbp\n");
}

Test(Plv, returns_ko_for_an_unknown_player_id)
{
    PlvFixture f;

    std::string res = f.commands->Plv({"999"}, *f.client);

    cr_assert_str_eq(res.c_str(), "ko\n");
}

Test(Plv, reports_level_one_for_a_freshly_hatched_player)
{
    PlvFixture f;

    std::string res = f.commands->Plv({std::to_string(f.playerId)}, *f.client);

    std::string expected = "plv " + std::to_string(f.playerId) + " 1\n";
    cr_assert_str_eq(res.c_str(), expected.c_str());
}

Test(Plv, buildPlvMessage_matches_the_command_reply)
{
    PlvFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);

    std::string viaCommand = f.commands->Plv({std::to_string(f.playerId)}, *f.client);
    std::string viaBuilder = f.commands->buildPlvMessage(*player);

    cr_assert_str_eq(viaCommand.c_str(), viaBuilder.c_str());
}

Test(Plv, successful_incantation_broadcasts_an_updated_plv_message)
{
    PlvFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    f.world.setTileAt(player->getPosition(), zappy::ItemType::LINEMATE, 1);

    f.commands->Incantation({}, *f.client, f.clients);

    cr_assert_eq(player->getLevel(), 2);
    // removeIncantationStones doesn't broadcast here since World has no broadcast
    // queue wired; the order is therefore: plv, pipi, pie.
    cr_assert_eq(f.broadcastQueue.size(), 3u);
    std::string expectedPlv = "plv " + std::to_string(player->getId()) + " 2\n";
    cr_assert_str_eq(f.broadcastQueue.front().c_str(), expectedPlv.c_str());
}
