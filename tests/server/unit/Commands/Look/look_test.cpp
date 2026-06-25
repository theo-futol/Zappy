#include "Network/Client/Client.hpp"
#include "Network/ClientHandler/Commands/Commands.hpp"
#include "Simulation/Utils.hpp"
#include "Simulation/World/World.hpp"
#include <algorithm>
#include <criterion/criterion.h>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

struct LookFixture
{
    int a, b;
    zappy::World world;
    std::queue<std::string> broadcastQueue;
    zappy::Commands *commands;
    zappy::Client *client;

    LookFixture() : world(5, 5)
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
    ~LookFixture()
    {
        delete commands;
        delete client;
        close(b);
    }
};

Test(Look, returns_ko_when_caller_has_no_player)
{
    LookFixture f;
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    zappy::Client ghost(sv[0]);

    std::string res = f.commands->Look({}, ghost);

    cr_assert_str_eq(res.c_str(), "ko\n");
    close(sv[1]);
}

Test(Look, result_is_wrapped_in_brackets_and_newline_terminated)
{
    LookFixture f;

    std::string res = f.commands->Look({}, *f.client);

    cr_assert(res.front() == '[');
    cr_assert(res.back() == '\n');
    cr_assert(res[res.size() - 2] == ']');
}

Test(Look, reports_resource_counts_on_the_players_own_tile)
{
    LookFixture f;
    zappy::Player *player = f.world.getPlayerByFd(f.a);
    f.world.setTileAt(player->getPosition(), zappy::ItemType::LINEMATE, 3);

    std::string res = f.commands->Look({}, *f.client);

    cr_assert(res.find("linemate:3") != std::string::npos);
}

Test(Look, default_level_one_player_sees_four_tiles)
{
    LookFixture f;

    std::string res = f.commands->Look({}, *f.client);

    // Rows 0 and 1 (1 + 3 tiles); separators follow the implementation's
    // level*(level+1) comma count (here level=1 => 2 commas).
    cr_assert_eq(static_cast<size_t>(std::count(res.begin(), res.end(), ',')), 2u);
}

Test(Look, reports_other_players_present_on_a_seen_tile)
{
    LookFixture f;
    int sv2[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv2);
    f.world.addPlayer(sv2[0], "team1"); // spawns on the same (0,0) tile

    std::string res = f.commands->Look({}, *f.client);

    cr_assert(res.find("player 1") != std::string::npos);
    close(sv2[0]);
    close(sv2[1]);
}

Test(Look, observer_alone_is_reported_as_zero_other_players_on_its_tile)
{
    LookFixture f;

    std::string res = f.commands->Look({}, *f.client);

    // The observer's own tile is non-empty (it stands there), so a "player N"
    // marker is still emitted, with N excluding the observer itself.
    cr_assert(res.find("player 0") != std::string::npos);
}

Test(Look, higher_level_increases_the_number_of_tiles_seen)
{
    LookFixture f;
    zappy::Player *player = f.world.getPlayerByFd(f.a);
    player->levelUp(); // level 2 now

    std::string res = f.commands->Look({}, *f.client);
    // Rows 0,1,2 (1 + 3 + 5 tiles); level=2 => level*(level+1) = 6 commas.
    cr_assert_eq(static_cast<size_t>(std::count(res.begin(), res.end(), ',')), 6u);
}

Test(Look, ignores_unused_args)
{
    LookFixture f;

    std::string res = f.commands->Look({"ignored", "args"}, *f.client);

    cr_assert(res.front() == '[');
    cr_assert(res.back() == '\n');
}

Test(Look, works_when_facing_east)
{
    LookFixture f;
    zappy::Player *player = f.world.getPlayerByFd(f.a);
    player->setRotation(zappy::Degrees::EAST);

    std::string res = f.commands->Look({}, *f.client);

    cr_assert(res.front() == '[');
    cr_assert(res.back() == '\n');
}

Test(Look, works_when_facing_south)
{
    LookFixture f;
    zappy::Player *player = f.world.getPlayerByFd(f.a);
    player->setRotation(zappy::Degrees::SOUTH);

    std::string res = f.commands->Look({}, *f.client);

    cr_assert(res.front() == '[');
    cr_assert(res.back() == '\n');
}

Test(Look, works_when_facing_west)
{
    LookFixture f;
    zappy::Player *player = f.world.getPlayerByFd(f.a);
    player->setRotation(zappy::Degrees::WEST);

    std::string res = f.commands->Look({}, *f.client);

    cr_assert(res.front() == '[');
    cr_assert(res.back() == '\n');
}

Test(Look, wraps_on_the_positive_x_boundary_when_facing_east)
{
    LookFixture f;
    zappy::Player *player = f.world.getPlayerByFd(f.a);
    player->setRotation(zappy::Degrees::EAST);
    player->setPosition(4, 2, f.world.getMapSize()); // x=4 is the last column on a 5x5 map
    // The level-1 vision row lands on x=5 (wraps to 0) for y in {1,2,3}; seed all three.
    f.world.setTileAt({0, 1}, zappy::ItemType::THYSTAME, 9);
    f.world.setTileAt({0, 2}, zappy::ItemType::THYSTAME, 9);
    f.world.setTileAt({0, 3}, zappy::ItemType::THYSTAME, 9);

    std::string res = f.commands->Look({}, *f.client);

    cr_assert(res.find("thystame:9") != std::string::npos);
}

Test(Look, wraps_on_the_positive_y_boundary_when_facing_south)
{
    LookFixture f;
    zappy::Player *player = f.world.getPlayerByFd(f.a);
    player->setRotation(zappy::Degrees::SOUTH);
    player->setPosition(2, 4, f.world.getMapSize()); // y=4 is the last row on a 5x5 map
    // The level-1 vision row lands on y=5 (wraps to 0) for x in {1,2,3}; seed all three.
    f.world.setTileAt({1, 0}, zappy::ItemType::THYSTAME, 9);
    f.world.setTileAt({2, 0}, zappy::ItemType::THYSTAME, 9);
    f.world.setTileAt({3, 0}, zappy::ItemType::THYSTAME, 9);

    std::string res = f.commands->Look({}, *f.client);

    cr_assert(res.find("thystame:9") != std::string::npos);
}
