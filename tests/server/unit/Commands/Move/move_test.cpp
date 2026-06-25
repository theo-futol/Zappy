#include "Network/Client/Client.hpp"
#include "Network/ClientHandler/Commands/Commands.hpp"
#include "Simulation/Utils.hpp"
#include "Simulation/World/World.hpp"
#include <criterion/criterion.h>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

struct MoveFixture
{
    int a, b;
    zappy::World world;
    std::queue<std::string> broadcastQueue;
    zappy::Commands *commands;
    zappy::Client *client;

    MoveFixture() : world(5, 5)
    {
        int sv[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
        a = sv[0];
        b = sv[1];
        client = new zappy::Client(a);
        world.addTeam("team1", 0, 1);
        world.addPlayer(a, "team1");
        commands = new zappy::Commands(&world, &broadcastQueue);
    }
    ~MoveFixture()
    {
        delete commands;
        delete client; // closes `a`
        close(b);
    }
};

Test(Move, forward_moves_the_player_one_tile_north_and_returns_ok)
{
    MoveFixture f;
    zappy::Player *player = f.world.getPlayerByFd(f.a);
    zappy::position before = player->getPosition();
    cr_assert_eq(before.x, 0);
    cr_assert_eq(before.y, 0);

    std::string res = f.commands->Forward({}, *f.client);

    cr_assert_str_eq(res.c_str(), "ok\n");
    zappy::position after = player->getPosition();
    // Facing NORTH by default, moving wraps to the bottom row on a 5x5 map.
    cr_assert_eq(after.x, 0);
    cr_assert_eq(after.y, 4);
}

Test(Move, forward_updates_the_tile_player_lists)
{
    MoveFixture f;
    zappy::Player *player = f.world.getPlayerByFd(f.a);
    zappy::position before = player->getPosition();

    f.commands->Forward({}, *f.client);

    zappy::tile *oldTile = f.world.getTileAt(before);
    zappy::tile *newTile = f.world.getTileAt(player->getPosition());
    cr_assert_eq(oldTile->_players.size(), 0u);
    cr_assert_eq(newTile->_players.size(), 1u);
}

Test(Move, forward_on_unknown_player_returns_dead)
{
    MoveFixture f;
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    zappy::Client ghost(sv[0]);

    std::string res = f.commands->Forward({}, ghost);

    cr_assert_str_eq(res.c_str(), "dead\n");
    close(sv[1]);
}

Test(Move, right_turns_90_degrees_clockwise)
{
    MoveFixture f;
    zappy::Player *player = f.world.getPlayerByFd(f.a);
    cr_assert_eq(player->getRotation(), zappy::Degrees::NORTH);

    std::string res = f.commands->Right({}, *f.client);

    cr_assert_str_eq(res.c_str(), "ok\n");
    cr_assert_eq(player->getRotation(), zappy::Degrees::EAST);
}

Test(Move, right_wraps_around_from_west_to_north)
{
    MoveFixture f;
    zappy::Player *player = f.world.getPlayerByFd(f.a);
    player->setRotation(zappy::Degrees::WEST);

    f.commands->Right({}, *f.client);

    cr_assert_eq(player->getRotation(), zappy::Degrees::NORTH);
}

Test(Move, right_on_unknown_player_returns_dead)
{
    MoveFixture f;
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    zappy::Client ghost(sv[0]);

    std::string res = f.commands->Right({}, ghost);

    cr_assert_str_eq(res.c_str(), "dead\n");
    close(sv[1]);
}

Test(Move, left_turns_90_degrees_counter_clockwise)
{
    MoveFixture f;
    zappy::Player *player = f.world.getPlayerByFd(f.a);
    player->setRotation(zappy::Degrees::EAST);

    std::string res = f.commands->Left({}, *f.client);

    cr_assert_str_eq(res.c_str(), "ok\n");
    cr_assert_eq(player->getRotation(), zappy::Degrees::NORTH);
}

Test(Move, left_wraps_around_from_north_to_west)
{
    MoveFixture f;
    zappy::Player *player = f.world.getPlayerByFd(f.a);
    cr_assert_eq(player->getRotation(), zappy::Degrees::NORTH);

    f.commands->Left({}, *f.client);

    cr_assert_eq(player->getRotation(), zappy::Degrees::WEST);
}

Test(Move, left_on_unknown_player_returns_dead)
{
    MoveFixture f;
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    zappy::Client ghost(sv[0]);

    std::string res = f.commands->Left({}, ghost);

    cr_assert_str_eq(res.c_str(), "dead\n");
    close(sv[1]);
}
