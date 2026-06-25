#include "Network/Client/Client.hpp"
#include "Network/ClientHandler/Commands/Commands.hpp"
#include "Simulation/Utils.hpp"
#include "Simulation/World/World.hpp"
#include <criterion/criterion.h>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

struct EjectFixture
{
    int a, b;
    zappy::World world;
    std::queue<std::string> broadcastQueue;
    zappy::Commands *commands;
    zappy::Client *client;

    EjectFixture() : world(5, 5)
    {
        int sv[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
        a = sv[0];
        b = sv[1];
        client = new zappy::Client(a);
        world.addTeam("team1", 0, 5);
        world.addPlayer(a, "team1");
        commands = new zappy::Commands(&world, &broadcastQueue);
    }
    ~EjectFixture()
    {
        delete commands;
        delete client;
        close(b);
    }
};

Test(Eject, returns_ko_when_caller_has_no_player)
{
    EjectFixture f;
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    zappy::Client ghost(sv[0]);

    std::string res = f.commands->Eject({}, ghost);

    cr_assert_str_eq(res.c_str(), "ko\n");
    close(sv[1]);
}

Test(Eject, returns_ko_when_alone_on_the_tile_with_no_egg)
{
    EjectFixture f;

    std::string res = f.commands->Eject({}, *f.client);

    cr_assert_str_eq(res.c_str(), "ko\n");
}

Test(Eject, ejects_another_player_on_the_same_tile_and_returns_ok)
{
    EjectFixture f;
    int sv2[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv2);
    f.world.addPlayer(sv2[0], "team1"); // spawns at (0,0), same tile as the ejector

    std::string res = f.commands->Eject({}, *f.client);

    cr_assert_str_eq(res.c_str(), "ok\n");
    zappy::Player *ejected = f.world.getPlayerByFd(sv2[0]);
    // The ejector faces NORTH by default, so the victim lands one tile north (wraps to y=4).
    cr_assert_eq(ejected->getPosition().x, 0);
    cr_assert_eq(ejected->getPosition().y, 4);

    close(sv2[0]);
    close(sv2[1]);
}

Test(Eject, moves_ejected_player_to_the_destination_tile_list)
{
    EjectFixture f;
    int sv2[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv2);
    f.world.addPlayer(sv2[0], "team1");
    zappy::Player *ejector = f.world.getPlayerByFd(f.a);
    zappy::Player *ejected = f.world.getPlayerByFd(sv2[0]);

    f.commands->Eject({}, *f.client);

    zappy::tile *originTile = f.world.getTileAt(ejector->getPosition());
    zappy::tile *destTile = f.world.getTileAt(ejected->getPosition());

    bool ejectedStillOnOrigin = false;
    for (auto *p : originTile->_players)
        if (p == ejected)
            ejectedStillOnOrigin = true;
    cr_assert_not(ejectedStillOnOrigin);

    bool ejectedOnDest = false;
    for (auto *p : destTile->_players)
        if (p == ejected)
            ejectedOnDest = true;
    cr_assert(ejectedOnDest);
}

Test(Eject, pushes_a_pex_event_with_the_ejector_fd)
{
    EjectFixture f;
    int sv2[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv2);
    f.world.addPlayer(sv2[0], "team1");

    f.commands->Eject({}, *f.client);

    cr_assert_eq(f.broadcastQueue.size(), 1u);
    std::string expected = "pex " + std::to_string(f.a) + "\n";
    cr_assert_str_eq(f.broadcastQueue.front().c_str(), expected.c_str());

    close(sv2[0]);
    close(sv2[1]);
}

Test(Eject, clears_eggs_on_the_tile_and_returns_ok)
{
    EjectFixture f;
    zappy::Player *player = f.world.getPlayerByFd(f.a);
    player->getTeam().addEgg(player->getPosition());
    cr_assert(player->getTeam().hasEggAtPosition(player->getPosition()));

    std::string res = f.commands->Eject({}, *f.client);

    cr_assert_str_eq(res.c_str(), "ok\n");
    cr_assert_not(player->getTeam().hasEggAtPosition(player->getPosition()));
}

Test(Eject, ignores_unused_args)
{
    EjectFixture f;
    int sv2[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv2);
    f.world.addPlayer(sv2[0], "team1");

    std::string res = f.commands->Eject({"ignored"}, *f.client);

    cr_assert_str_eq(res.c_str(), "ok\n");
    close(sv2[0]);
    close(sv2[1]);
}

Test(Eject, returns_ko_on_a_1x1_map_where_forward_movement_cannot_displace_anyone)
{
    // On a 1x1 map nextPosition() wraps back onto the same tile, so the other
    // player can never be moved away: hasEjectedPlayers stays false.
    zappy::World world(1, 1);
    std::queue<std::string> broadcastQueue;
    int sv[2], sv2[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv2);
    zappy::Client client(sv[0]);

    world.addTeam("team1", 0, 5);
    world.addPlayer(sv[0], "team1");
    world.addPlayer(sv2[0], "team1");
    zappy::Commands commands(&world, &broadcastQueue);

    std::string res = commands.Eject({}, client);

    cr_assert_str_eq(res.c_str(), "ko\n");
    close(sv[1]);
    close(sv2[0]);
    close(sv2[1]);
}
