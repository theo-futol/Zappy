#include "Network/Client/Client.hpp"
#include "Network/ClientHandler/Commands/Commands.hpp"
#include "Simulation/World/World.hpp"
#include <criterion/criterion.h>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

struct ConnectFixture
{
    int a, b;
    zappy::World world;
    std::queue<std::string> broadcastQueue;
    zappy::Commands *commands;
    zappy::Client *client;

    ConnectFixture() : world(5, 5)
    {
        int sv[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
        a = sv[0];
        b = sv[1];
        client = new zappy::Client(a);
        commands = new zappy::Commands(&world, &broadcastQueue);
    }
    ~ConnectFixture()
    {
        delete commands;
        delete client;
        close(b);
    }
};

Test(Connect, returns_ko_when_the_caller_has_no_player)
{
    ConnectFixture f;

    std::string res = f.commands->Connect_nbr({}, *f.client);

    cr_assert_str_eq(res.c_str(), "ko\n");
}

Test(Connect, returns_all_slots_when_team_is_empty)
{
    ConnectFixture f;
    f.world.addTeam("team1", 0, 3);
    f.world.addPlayer(f.a, "team1");

    std::string res = f.commands->Connect_nbr({}, *f.client);

    // 3 initial slots, one taken by this player => 2 left.
    cr_assert_str_eq(res.c_str(), "2\n");
}

Test(Connect, decreases_as_more_players_join_the_team)
{
    ConnectFixture f;
    f.world.addTeam("team1", 0, 3);
    f.world.addPlayer(f.a, "team1");

    int sv2[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv2);
    f.world.addPlayer(sv2[0], "team1");

    std::string res = f.commands->Connect_nbr({}, *f.client);

    cr_assert_str_eq(res.c_str(), "1\n");
    close(sv2[0]);
    close(sv2[1]);
}

Test(Connect, returns_zero_when_team_is_full)
{
    ConnectFixture f;
    f.world.addTeam("team1", 0, 1);
    f.world.addPlayer(f.a, "team1");

    std::string res = f.commands->Connect_nbr({}, *f.client);

    cr_assert_str_eq(res.c_str(), "0\n");
}

Test(Connect, ignores_unused_args)
{
    ConnectFixture f;
    f.world.addTeam("team1", 0, 5);
    f.world.addPlayer(f.a, "team1");

    std::string res = f.commands->Connect_nbr({"unexpected", "args"}, *f.client);

    cr_assert_str_eq(res.c_str(), "4\n");
}
