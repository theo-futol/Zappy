#include "Network/Client/Client.hpp"
#include "Network/ClientHandler/Commands/Commands.hpp"
#include "Simulation/Utils.hpp"
#include "Simulation/World/World.hpp"
#include <criterion/criterion.h>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

struct ItemsFixture
{
    int a, b;
    zappy::World world;
    std::queue<std::string> broadcastQueue;
    std::vector<std::unique_ptr<zappy::Client>> clients;
    zappy::Commands *commands;
    zappy::Client *client;
    int playerId;

    ItemsFixture() : world(5, 5, 100)
    {
        int sv[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
        a = sv[0];
        b = sv[1];
        client = new zappy::Client(a);
        world.addTeam("team1", 0, 5);
        playerId = world.addPlayer(a, "team1");
        client->setPlayerId(playerId);
        zappy::Player *spawned = world.getPlayerById(playerId);
        world.removePlayerFromTile(spawned, spawned->getPosition());
        spawned->setPosition(0, 0, world.getMapSize());
        world.addPlayerToTile(spawned, spawned->getPosition());
        commands = new zappy::Commands(&world, &broadcastQueue);
    }
    ~ItemsFixture()
    {
        delete commands;
        delete client;
        close(b);
    }

    int tileItemCount(zappy::ItemType type)
    {
        zappy::Player *player = world.getPlayerById(playerId);
        zappy::tile *tilePtr = world.getTileAt(player->getPosition());
        for (const auto &item : tilePtr->_items)
            if (item.first == type)
                return item.second;
        return -1;
    }
};

Test(Take, returns_ko_when_no_args_are_given)
{
    ItemsFixture f;

    std::string res = f.commands->Take({}, *f.client, f.clients);

    cr_assert_str_eq(res.c_str(), "ko\n");
}

Test(Take, returns_ko_when_caller_has_no_tile)
{
    ItemsFixture f;
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    zappy::Client ghost(sv[0]);

    std::string res = f.commands->Take({"food"}, ghost, f.clients);

    cr_assert_str_eq(res.c_str(), "ko\n");
    close(sv[0]);
    close(sv[1]);
}

Test(Take, returns_ko_for_an_unknown_item_name)
{
    ItemsFixture f;

    std::string res = f.commands->Take({"notanitem"}, *f.client, f.clients);

    cr_assert_str_eq(res.c_str(), "ko\n");
}

Test(Take, returns_ko_when_the_item_is_not_present_on_the_tile)
{
    ItemsFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    // Passive resource generation seeds the map randomly; force the tile empty.
    f.world.setTileAt(player->getPosition(), zappy::ItemType::LINEMATE, 0);

    std::string res = f.commands->Take({"linemate"}, *f.client, f.clients);

    cr_assert_str_eq(res.c_str(), "ko\n");
}

Test(Take, picks_up_an_item_updates_inventory_and_tile_and_broadcasts_pgt)
{
    ItemsFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    f.world.setTileAt(player->getPosition(), zappy::ItemType::LINEMATE, 2);

    std::string res = f.commands->Take({"linemate"}, *f.client, f.clients);

    cr_assert_str_eq(res.c_str(), "ok\n");
    cr_assert_eq(f.tileItemCount(zappy::ItemType::LINEMATE), 1);
    cr_assert_eq(player->getInventory().getItemCount(zappy::ItemType::LINEMATE), 1);

    cr_assert_eq(f.broadcastQueue.size(), 4u);
    std::string expectedPgt = "pgt " + std::to_string(player->getId()) + " " + std::to_string(static_cast<int>(zappy::ItemType::LINEMATE)) + "\n";
    cr_assert_str_eq(f.broadcastQueue.front().c_str(), expectedPgt.c_str());
    f.broadcastQueue.pop();

    std::string expectedPipi = "pipi " + std::to_string(player->getId()) + " " + std::to_string(player->getPosition().x) + " " + std::to_string(player->getPosition().y) + " " +
                               std::to_string(player->getOrientation()) + " " + std::to_string(player->getLevel()) + " 10 1 0 0 0 0 0 \n";
    cr_assert_str_eq(f.broadcastQueue.front().c_str(), expectedPipi.c_str());
    f.broadcastQueue.pop();

    std::string expectedPin =
        "pin " + std::to_string(player->getId()) + " " + std::to_string(player->getPosition().x) + " " + std::to_string(player->getPosition().y) + " 10 1 0 0 0 0 0 \n";
    cr_assert_str_eq(f.broadcastQueue.front().c_str(), expectedPin.c_str());
    f.broadcastQueue.pop();

    std::string expectedBct = f.commands->Bct({"bct", std::to_string(player->getPosition().x), std::to_string(player->getPosition().y)}, *f.client);
    cr_assert_str_eq(f.broadcastQueue.front().c_str(), expectedBct.c_str());
}

Test(Set, returns_ko_when_no_args_are_given)
{
    ItemsFixture f;

    std::string res = f.commands->Set({}, *f.client, f.clients);

    cr_assert_str_eq(res.c_str(), "ko\n");
}

Test(Set, returns_ko_when_caller_has_no_tile)
{
    ItemsFixture f;
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    zappy::Client ghost(sv[0]);

    std::string res = f.commands->Set({"food"}, ghost, f.clients);

    cr_assert_str_eq(res.c_str(), "ko\n");
    close(sv[0]);
    close(sv[1]);
}

Test(Set, returns_ko_for_an_unknown_item_name)
{
    ItemsFixture f;

    std::string res = f.commands->Set({"notanitem"}, *f.client, f.clients);

    cr_assert_str_eq(res.c_str(), "ko\n");
}

Test(Set, returns_ko_when_the_player_does_not_have_the_item)
{
    ItemsFixture f;

    std::string res = f.commands->Set({"linemate"}, *f.client, f.clients);

    cr_assert_str_eq(res.c_str(), "ko\n");
}

Test(Set, drops_an_item_updates_inventory_and_tile_and_broadcasts_pdr)
{
    ItemsFixture f;
    zappy::Player *player = f.world.getPlayerById(f.playerId);
    // Passive resource generation seeds the map randomly; force the tile empty first.
    f.world.setTileAt(player->getPosition(), zappy::ItemType::LINEMATE, 0);
    player->getInventory().addItem(zappy::ItemType::LINEMATE, 2);

    std::string res = f.commands->Set({"linemate"}, *f.client, f.clients);

    cr_assert_str_eq(res.c_str(), "ok\n");
    cr_assert_eq(player->getInventory().getItemCount(zappy::ItemType::LINEMATE), 1);
    cr_assert_eq(f.tileItemCount(zappy::ItemType::LINEMATE), 1);

    cr_assert_eq(f.broadcastQueue.size(), 4u);
    std::string expectedPdr = "pdr " + std::to_string(player->getId()) + " " + std::to_string(static_cast<int>(zappy::ItemType::LINEMATE)) + "\n";
    cr_assert_str_eq(f.broadcastQueue.front().c_str(), expectedPdr.c_str());
    f.broadcastQueue.pop();

    std::string expectedPipi = "pipi " + std::to_string(player->getId()) + " " + std::to_string(player->getPosition().x) + " " + std::to_string(player->getPosition().y) + " " +
                               std::to_string(player->getOrientation()) + " " + std::to_string(player->getLevel()) + " 10 1 0 0 0 0 0 \n";
    cr_assert_str_eq(f.broadcastQueue.front().c_str(), expectedPipi.c_str());
    f.broadcastQueue.pop();

    std::string expectedPin =
        "pin " + std::to_string(player->getId()) + " " + std::to_string(player->getPosition().x) + " " + std::to_string(player->getPosition().y) + " 10 1 0 0 0 0 0 \n";
    cr_assert_str_eq(f.broadcastQueue.front().c_str(), expectedPin.c_str());
    f.broadcastQueue.pop();

    std::string expectedBct = f.commands->Bct({"bct", std::to_string(player->getPosition().x), std::to_string(player->getPosition().y)}, *f.client);
    cr_assert_str_eq(f.broadcastQueue.front().c_str(), expectedBct.c_str());
}
