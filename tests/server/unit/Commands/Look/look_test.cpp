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
    std::vector<std::unique_ptr<zappy::Client>> clients;
    zappy::Commands *commands;
    zappy::Client *client;
    int playerId;

    LookFixture() : world(5, 5, 100)
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
        // are deterministic, keeping the tile player-lists consistent.
        zappy::Player *spawned = world.getPlayerById(playerId);
        world.removePlayerFromTile(spawned, spawned->getPosition());
        spawned->setPosition(0, 0, world.getMapSize());
        world.addPlayerToTile(spawned, spawned->getPosition());
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

    std::string res = f.commands->Look({}, ghost, f.clients);

    cr_assert_str_eq(res.c_str(), "ko\n");
    close(sv[1]);
}

Test(Look, result_is_wrapped_in_brackets_and_newline_terminated)
{
    LookFixture f;

    std::string res = f.commands->Look({}, *f.client, f.clients);

    cr_assert(res.front() == '[');
    cr_assert(res.back() == '\n');
    cr_assert(res[res.size() - 2] == ']');
}

Test(Look, reports_resource_counts_on_the_players_own_tile)
{
    LookFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    f.world.setTileAt(player->getPosition(), zappy::ItemType::LINEMATE, 3);

    std::string res = f.commands->Look({}, *f.client, f.clients);

    cr_assert(res.find("linemate:3") != std::string::npos);
}

Test(Look, default_level_one_player_sees_four_tiles)
{
    LookFixture f;

    std::string res = f.commands->Look({}, *f.client, f.clients);

    // Rows 0 and 1 (1 + 3 = 4 tiles) are joined by (4 - 1) = 3 separators; the
    // implementation emits one "," between every pair of tiles seen.
    cr_assert_eq(static_cast<size_t>(std::count(res.begin(), res.end(), ',')), 3u);
}

Test(Look, reports_other_players_present_on_a_seen_tile)
{
    LookFixture f;
    int sv2[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv2);
    int otherId = f.world.addPlayer(sv2[0], "team1");
    // Eggs hatch at a random tile; pin the new player onto the observer's own tile.
    zappy::Player *other = f.world.getPlayerById(otherId);
    f.world.removePlayerFromTile(other, other->getPosition());
    other->setPosition(0, 0, f.world.getMapSize());
    f.world.addPlayerToTile(other, other->getPosition());

    std::string res = f.commands->Look({}, *f.client, f.clients);

    // The implementation does not number players; it emits one "player " token
    // per player standing on the tile (including the observer itself).
    size_t playerTokens = 0;
    for (size_t pos = res.find("player "); pos != std::string::npos; pos = res.find("player ", pos + 1))
        ++playerTokens;
    cr_assert_eq(playerTokens, 2u);
    close(sv2[0]);
    close(sv2[1]);
}

Test(Look, observer_alone_is_reported_as_present_on_its_own_tile)
{
    LookFixture f;

    std::string res = f.commands->Look({}, *f.client, f.clients);

    // The observer's own tile is non-empty (it stands there), so exactly one
    // "player " marker is emitted for that tile.
    cr_assert(res.find("player ") != std::string::npos);
}

Test(Look, higher_level_increases_the_number_of_tiles_seen)
{
    LookFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    player->levelUp(); // level 2 now

    std::string res = f.commands->Look({}, *f.client, f.clients);
    // Rows 0,1,2 (1 + 3 + 5 = 9 tiles) are joined by (9 - 1) = 8 separators.
    cr_assert_eq(static_cast<size_t>(std::count(res.begin(), res.end(), ',')), 8u);
}

Test(Look, ignores_unused_args)
{
    LookFixture f;

    std::string res = f.commands->Look({"ignored", "args"}, *f.client, f.clients);

    cr_assert(res.front() == '[');
    cr_assert(res.back() == '\n');
}

Test(Look, works_when_facing_east)
{
    LookFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    player->setRotation(zappy::Degrees::EAST);

    std::string res = f.commands->Look({}, *f.client, f.clients);

    cr_assert(res.front() == '[');
    cr_assert(res.back() == '\n');
}

Test(Look, works_when_facing_south)
{
    LookFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    player->setRotation(zappy::Degrees::SOUTH);

    std::string res = f.commands->Look({}, *f.client, f.clients);

    cr_assert(res.front() == '[');
    cr_assert(res.back() == '\n');
}

Test(Look, works_when_facing_west)
{
    LookFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    player->setRotation(zappy::Degrees::WEST);

    std::string res = f.commands->Look({}, *f.client, f.clients);

    cr_assert(res.front() == '[');
    cr_assert(res.back() == '\n');
}

Test(Look, wraps_on_the_positive_x_boundary_when_facing_east)
{
    LookFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    player->setRotation(zappy::Degrees::EAST);
    player->setPosition(4, 2, f.world.getMapSize()); // x=4 is the last column on a 5x5 map
    // The level-1 vision row lands on x=5 (wraps to 0) for y in {1,2,3}; seed all three.
    f.world.setTileAt({0, 1}, zappy::ItemType::THYSTAME, 9);
    f.world.setTileAt({0, 2}, zappy::ItemType::THYSTAME, 9);
    f.world.setTileAt({0, 3}, zappy::ItemType::THYSTAME, 9);

    std::string res = f.commands->Look({}, *f.client, f.clients);

    cr_assert(res.find("thystame:9") != std::string::npos);
}

Test(Look, wraps_on_the_positive_y_boundary_when_facing_south)
{
    LookFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    player->setRotation(zappy::Degrees::SOUTH);
    player->setPosition(2, 4, f.world.getMapSize()); // y=4 is the last row on a 5x5 map
    // The level-1 vision row lands on y=5 (wraps to 0) for x in {1,2,3}; seed all three.
    f.world.setTileAt({1, 0}, zappy::ItemType::THYSTAME, 9);
    f.world.setTileAt({2, 0}, zappy::ItemType::THYSTAME, 9);
    f.world.setTileAt({3, 0}, zappy::ItemType::THYSTAME, 9);

    std::string res = f.commands->Look({}, *f.client, f.clients);

    cr_assert(res.find("thystame:9") != std::string::npos);
}
