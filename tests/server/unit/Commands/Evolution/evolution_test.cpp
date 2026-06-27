#include "Network/Client/Client.hpp"
#include "Network/ClientHandler/Commands/Commands.hpp"
#include "Simulation/Utils.hpp"
#include "Simulation/World/World.hpp"
#include <chrono>
#include <criterion/criterion.h>
#include <fcntl.h>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

struct EvolutionFixture
{
    int a, b;
    zappy::World world;
    std::queue<std::string> broadcastQueue;
    std::vector<std::unique_ptr<zappy::Client>> clients;
    zappy::Commands *commands;
    zappy::Client *client;
    int playerId;

    EvolutionFixture() : world(5, 5, 100)
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
    ~EvolutionFixture()
    {
        delete commands;
        // client is owned by clients[0]; the vector destructor closes the fd.
        close(b);
    }

    std::string read_reply()
    {
        char buf[4096] = {0};
        int n = recv(b, buf, sizeof(buf) - 1, 0);
        return n > 0 ? std::string(buf, n) : std::string();
    }
};

Test(Fork, returns_ko_when_caller_has_no_player)
{
    EvolutionFixture f;
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    zappy::Client ghost(sv[0]);

    std::string res = f.commands->Fork({}, ghost, f.clients);

    cr_assert_str_eq(res.c_str(), "ko\n");
    close(sv[0]);
    close(sv[1]);
}

Test(Fork, lays_an_egg_at_the_player_position_and_returns_ok)
{
    EvolutionFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);

    std::string res = f.commands->Fork({}, *f.client, f.clients);

    cr_assert_str_eq(res.c_str(), "ok\n");
    zappy::tile *tilePtr = f.world.getTileAt(player->getPosition());
    bool hasEgg = false;
    for (const auto &item : tilePtr->_items)
        if (item.first == zappy::ItemType::EGG && item.second == 1)
            hasEgg = true;
    cr_assert(hasEgg);
    cr_assert(player->getTeam().hasEggAtPosition(player->getPosition()));
}

Test(Evolution, beginIncantation_returns_false_when_caller_has_no_player)
{
    EvolutionFixture f;
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    zappy::Client ghost(sv[0]);

    bool started = f.commands->beginIncantation(ghost, std::chrono::steady_clock::now(), f.clients);

    cr_assert_not(started);
    close(sv[0]);
    close(sv[1]);
}

Test(Evolution, beginIncantation_fails_when_conditions_are_not_met)
{
    EvolutionFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    // No linemate stone on the tile: requirement for level 1 -> 2 is not met.
    f.world.setTileAt(player->getPosition(), zappy::ItemType::LINEMATE, 0);

    bool started = f.commands->beginIncantation(*f.client, std::chrono::steady_clock::now(), f.clients);

    cr_assert_not(started);
    zappy::tile *tilePtr = f.world.getTileAt(player->getPosition());
    cr_assert_not(tilePtr->_incantationInProgress);

    std::string reply = f.read_reply();
    cr_assert_str_eq(reply.c_str(), "ko\n");
}

Test(Evolution, beginIncantation_succeeds_and_freezes_participants)
{
    EvolutionFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    f.world.setTileAt(player->getPosition(), zappy::ItemType::LINEMATE, 1);

    bool started = f.commands->beginIncantation(*f.client, std::chrono::steady_clock::now() + std::chrono::seconds(10), f.clients);

    cr_assert(started);
    zappy::tile *tilePtr = f.world.getTileAt(player->getPosition());
    cr_assert(tilePtr->_incantationInProgress);
    cr_assert(player->isFrozen());

    cr_assert_eq(f.broadcastQueue.size(), 1u);
    cr_assert(f.broadcastQueue.front().rfind("pic ", 0) == 0);

    std::string reply = f.read_reply();
    cr_assert_str_eq(reply.c_str(), "Elevation underway\n");
}

Test(Incantation, returns_ko_when_caller_has_no_player)
{
    EvolutionFixture f;
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    zappy::Client ghost(sv[0]);

    std::string res = f.commands->Incantation({}, ghost, f.clients);

    cr_assert_str_eq(res.c_str(), "ko\n");
    close(sv[0]);
    close(sv[1]);
}

Test(Incantation, fails_when_conditions_are_not_met_and_notifies_participants)
{
    EvolutionFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    f.world.setTileAt(player->getPosition(), zappy::ItemType::LINEMATE, 0);

    std::string res = f.commands->Incantation({}, *f.client, f.clients);

    cr_assert(res.empty());
    zappy::tile *tilePtr = f.world.getTileAt(player->getPosition());
    cr_assert_not(tilePtr->_incantationInProgress);
    cr_assert_eq(player->getLevel(), 1);

    std::string reply = f.read_reply();
    cr_assert_str_eq(reply.c_str(), "ko\n");

    cr_assert_eq(f.broadcastQueue.size(), 1u);
    std::string expected = "pie " + std::to_string(player->getPosition().x) + " " + std::to_string(player->getPosition().y) + " 0\n";
    cr_assert_str_eq(f.broadcastQueue.front().c_str(), expected.c_str());
}

Test(Incantation, succeeds_levels_up_participants_and_consumes_stones)
{
    EvolutionFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    f.world.setTileAt(player->getPosition(), zappy::ItemType::LINEMATE, 1);

    std::string res = f.commands->Incantation({}, *f.client, f.clients);

    cr_assert(res.empty());
    cr_assert_eq(player->getLevel(), 2);

    zappy::tile *tilePtr = f.world.getTileAt(player->getPosition());
    cr_assert_not(tilePtr->_incantationInProgress);
    int linemate = 0;
    for (const auto &item : tilePtr->_items)
        if (item.first == zappy::ItemType::LINEMATE)
            linemate = item.second;
    cr_assert_eq(linemate, 0);

    std::string reply = f.read_reply();
    cr_assert_str_eq(reply.c_str(), "Current level: 2\n");

    cr_assert_eq(f.broadcastQueue.size(), 2u);
    std::string plvMsg = f.broadcastQueue.front();
    f.broadcastQueue.pop();
    std::string expectedPlv = "plv " + std::to_string(player->getId()) + " 2\n";
    cr_assert_str_eq(plvMsg.c_str(), expectedPlv.c_str());

    std::string pieMsg = f.broadcastQueue.front();
    std::string expectedPie = "pie " + std::to_string(player->getPosition().x) + " " + std::to_string(player->getPosition().y) + " 1\n";
    cr_assert_str_eq(pieMsg.c_str(), expectedPie.c_str());
}
