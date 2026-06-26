#include "Network/Client/Client.hpp"
#include "Network/ClientHandler/CommandParser/CommandParser.hpp"
#include "Simulation/World/World.hpp"
#include <chrono>
#include <criterion/criterion.h>
#include <fcntl.h>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>

struct ParserFixture
{
    int a, b; // a: server-side fd wrapped by Client; b: peer used by the test to write/read.
    zappy::World world;
    std::queue<std::string> broadcastQueue;
    zappy::Client *client;
    zappy::CommandParser *parser;

    ParserFixture() : world(10, 10, 100)
    {
        int sv[2];
        socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
        a = sv[0];
        b = sv[1];
        int flags = fcntl(b, F_GETFL, 0);
        fcntl(b, F_SETFL, flags | O_NONBLOCK);
        world.addTeam("team1", 0, 5);
        client = new zappy::Client(a);
        parser = new zappy::CommandParser(client, &world, &broadcastQueue);
    }
    ~ParserFixture()
    {
        delete parser;
        delete client;
        close(b);
    }

    void send_line(const std::string &line)
    {
        std::string withNl = line + "\n";
        write(b, withNl.c_str(), withNl.size());
    }

    std::string read_reply()
    {
        char buf[4096] = {0};
        int n = recv(b, buf, sizeof(buf) - 1, 0);
        return n > 0 ? std::string(buf, n) : std::string();
    }
};

Test(CommandParser, feed_returns_false_when_peer_closed_the_connection)
{
    ParserFixture f;
    close(f.b);
    f.b = -1;

    cr_assert_not(f.parser->feed());
}

Test(CommandParser, feed_returns_true_when_data_was_read)
{
    ParserFixture f;
    f.send_line("team1");

    cr_assert(f.parser->feed());
}

Test(CommandParser, executeNext_on_empty_queue_returns_false)
{
    ParserFixture f;

    cr_assert_not(f.parser->executeNext());
}

Test(CommandParser, nextReadyAt_is_max_when_queue_is_empty)
{
    ParserFixture f;

    cr_assert_eq(f.parser->nextReadyAt(), std::chrono::steady_clock::time_point::max());
}

Test(CommandParser, nextReadyAt_is_finite_once_a_command_is_queued)
{
    ParserFixture f;
    f.send_line("team1");
    f.parser->feed();

    cr_assert_lt(f.parser->nextReadyAt(), std::chrono::steady_clock::time_point::max());
}

Test(CommandParser, handshake_with_known_team_promotes_client_to_AI_and_replies_with_slots_and_map_size)
{
    ParserFixture f;
    f.send_line("team1");
    f.parser->feed();

    cr_assert(f.parser->executeNext());
    cr_assert_eq(f.client->getType(), zappy::ClientType::AI);

    std::string reply = f.read_reply();
    // The slot count reported is the value *before* this connecting player
    // occupies a slot (computed first, then addPlayer() is called).
    cr_assert_str_eq(reply.c_str(), "5\n10 10\n");
}

Test(CommandParser, handshake_with_GRAPHIC_promotes_client_to_GRAPHIC_and_sends_no_reply)
{
    ParserFixture f;
    f.send_line("GRAPHIC");
    f.parser->feed();

    cr_assert(f.parser->executeNext());
    cr_assert_eq(f.client->getType(), zappy::ClientType::GRAPHIC);

    std::string reply = f.read_reply();
    cr_assert(reply.empty());
}

Test(CommandParser, handshake_with_unknown_team_replies_ko_and_keeps_client_AI)
{
    ParserFixture f;
    f.send_line("unknown_team");
    f.parser->feed();

    cr_assert(f.parser->executeNext());
    cr_assert_eq(f.client->getType(), zappy::ClientType::AI);

    std::string reply = f.read_reply();
    cr_assert_str_eq(reply.c_str(), "ko\n");
}

Test(CommandParser, handshake_pushes_a_pnw_event_to_the_broadcast_queue)
{
    ParserFixture f;
    f.send_line("team1");
    f.parser->feed();
    f.parser->executeNext();

    cr_assert_eq(f.broadcastQueue.size(), 1u);
    cr_assert(f.broadcastQueue.front().rfind("pnw ", 0) == 0);
}

Test(CommandParser, known_AI_command_is_dispatched_and_replies_ok)
{
    ParserFixture f;
    f.send_line("team1");
    f.parser->feed();
    f.parser->executeNext(); // handshake
    f.read_reply();

    f.send_line("Right");
    f.parser->feed();
    // "Right" costs 7/f = 70ms at f=100; wait for it to become due.
    std::this_thread::sleep_for(std::chrono::milliseconds(80));
    cr_assert(f.parser->executeNext());

    std::string reply = f.read_reply();
    cr_assert_str_eq(reply.c_str(), "ok\n");
}

Test(CommandParser, unknown_AI_command_replies_ko)
{
    ParserFixture f;
    f.send_line("team1");
    f.parser->feed();
    f.parser->executeNext();
    f.read_reply();

    f.send_line("NotACommand");
    f.parser->feed();
    f.parser->executeNext();

    std::string reply = f.read_reply();
    cr_assert_str_eq(reply.c_str(), "ko\n");
}

Test(CommandParser, unknown_graphic_command_replies_suc)
{
    ParserFixture f;
    f.send_line("GRAPHIC");
    f.parser->feed();
    f.parser->executeNext();

    f.send_line("not_a_graphic_cmd");
    f.parser->feed();
    f.parser->executeNext();

    std::string reply = f.read_reply();
    cr_assert_str_eq(reply.c_str(), "suc\n");
}

Test(CommandParser, known_graphic_command_is_dispatched)
{
    ParserFixture f;
    f.send_line("GRAPHIC");
    f.parser->feed();
    f.parser->executeNext();

    f.send_line("msz");
    f.parser->feed();
    f.parser->executeNext();

    std::string reply = f.read_reply();
    cr_assert_str_eq(reply.c_str(), "msz 10 10\n");
}

Test(CommandParser, dead_client_is_told_dead_and_command_is_dropped)
{
    ParserFixture f;
    f.client->setType(zappy::ClientType::DEAD);
    f.send_line("Forward");
    f.parser->feed();

    cr_assert(f.parser->executeNext());

    std::string reply = f.read_reply();
    cr_assert_str_eq(reply.c_str(), "dead\n");
}

Test(CommandParser, is_not_banned_initially)
{
    ParserFixture f;

    cr_assert_not(f.parser->isBanned());
}

Test(CommandParser, queuing_more_than_ten_commands_bans_the_client)
{
    ParserFixture f;
    f.send_line("team1");
    f.parser->feed();
    f.parser->executeNext();
    f.read_reply();

    for (int i = 0; i < 12; ++i)
        f.send_line("Right");
    f.parser->feed();

    cr_assert_not(f.parser->executeNext());
    cr_assert(f.parser->isBanned());
}

Test(CommandParser, empty_lines_are_ignored)
{
    ParserFixture f;
    write(f.b, "\n\n", 2);
    f.parser->feed();

    cr_assert_eq(f.parser->nextReadyAt(), std::chrono::steady_clock::time_point::max());
}

Test(CommandParser, command_not_yet_due_is_not_executed)
{
    ParserFixture f;
    f.send_line("team1");
    f.parser->feed();
    f.parser->executeNext(); // handshake, immediate (cost 0)
    f.read_reply();

    f.send_line("Forward"); // costs 7/f = 70ms at f=100
    f.parser->feed();

    cr_assert_not(f.parser->executeNext());
}

Test(CommandParser, frozen_AI_player_cannot_execute_queued_commands)
{
    ParserFixture f;
    f.send_line("team1");
    f.parser->feed();
    f.parser->executeNext();
    f.read_reply();

    zappy::Player *player = f.world.getPlayerById(f.client->getPlayerId());
    player->setFrozenUntil(std::chrono::steady_clock::now() + std::chrono::seconds(10));

    f.send_line("Connect_nbr"); // cost 0, would otherwise be immediately due
    f.parser->feed();

    cr_assert_not(f.parser->executeNext());
}

Test(CommandParser, incantation_command_is_dropped_when_requirements_are_not_met)
{
    ParserFixture f;
    f.send_line("team1");
    f.parser->feed();
    f.parser->executeNext(); // handshake: spawns a level-1 player alone on its tile
    f.read_reply();

    // Passive resource generation seeds the map randomly; make sure the spawn
    // tile has no linemate stone so the elevation requirement genuinely fails.
    zappy::Player *player = f.world.getPlayerById(f.client->getPlayerId());
    f.world.setTileAt(player->getPosition(), zappy::ItemType::LINEMATE, 0);

    f.send_line("Incantation");
    f.parser->feed();

    // beginIncantation() fails (no linemate stone, alone on the tile), so the
    // command must be skipped entirely rather than queued.
    cr_assert_eq(f.parser->nextReadyAt(), std::chrono::steady_clock::time_point::max());

    std::string reply = f.read_reply();
    cr_assert_str_eq(reply.c_str(), "ko\n");
}

Test(CommandParser, dispatched_command_with_an_argument_is_parsed_correctly)
{
    ParserFixture f;
    f.send_line("team1");
    f.parser->feed();
    f.parser->executeNext();
    f.read_reply();

    f.send_line("Broadcast hello");
    f.parser->feed();
    std::this_thread::sleep_for(std::chrono::milliseconds(80)); // Broadcast costs 7/f
    cr_assert(f.parser->executeNext());

    std::string reply = f.read_reply();
    cr_assert_str_eq(reply.c_str(), "ok\n");
}
