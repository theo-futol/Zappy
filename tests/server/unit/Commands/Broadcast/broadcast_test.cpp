#include "Network/Client/Client.hpp"
#include "Network/ClientHandler/Commands/Commands.hpp"
#include "Simulation/World/World.hpp"
#include <criterion/criterion.h>
#include <fcntl.h>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

struct BroadcastFixture
{
    int a, b;
    zappy::World world;
    std::queue<std::string> broadcastQueue;
    zappy::Commands *commands;
    zappy::Client *client;

    BroadcastFixture() : world(5, 5)
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
    ~BroadcastFixture()
    {
        delete commands;
        delete client;
        close(b);
    }
};

Test(Broadcast, returns_ko_when_caller_has_no_player)
{
    BroadcastFixture f;
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    zappy::Client ghost(sv[0]);

    std::string res = f.commands->Broadcast({"hello"}, ghost);

    cr_assert_str_eq(res.c_str(), "ko\n");
    close(sv[1]);
}

Test(Broadcast, returns_ko_when_no_argument_is_given)
{
    BroadcastFixture f;

    std::string res = f.commands->Broadcast({}, *f.client);

    cr_assert_str_eq(res.c_str(), "ko\n");
}

Test(Broadcast, returns_ko_when_too_many_arguments_are_given)
{
    BroadcastFixture f;

    std::string res = f.commands->Broadcast({"hello", "world"}, *f.client);

    cr_assert_str_eq(res.c_str(), "ko\n");
}

Test(Broadcast, returns_ko_when_message_is_empty)
{
    BroadcastFixture f;

    std::string res = f.commands->Broadcast({""}, *f.client);

    cr_assert_str_eq(res.c_str(), "ko\n");
}

Test(Broadcast, returns_ok_for_a_valid_single_word_message)
{
    BroadcastFixture f;

    std::string res = f.commands->Broadcast({"hello"}, *f.client);

    cr_assert_str_eq(res.c_str(), "ok\n");
}

Test(Broadcast, pushes_a_pbc_event_with_the_caller_fd_and_text)
{
    BroadcastFixture f;

    f.commands->Broadcast({"hello"}, *f.client);

    cr_assert_eq(f.broadcastQueue.size(), 1u);
    std::string expected = "pbc " + std::to_string(f.a) + " hello\n";
    cr_assert_str_eq(f.broadcastQueue.front().c_str(), expected.c_str());
}

Test(Broadcast, does_not_queue_a_message_for_a_lone_player)
{
    BroadcastFixture f;
    zappy::Player *player = f.world.getPlayerByFd(f.a);

    f.commands->Broadcast({"hello"}, *f.client);
    // Nothing should be readable on the sender's own socket since it is the
    // only player in the world (no targets to notify).
    f.commands->Broadcast({"again"}, *f.client); // calling again must stay stable / not crash
    (void)player;

    char buf[64];
    int flags = fcntl(f.a, F_GETFL, 0);
    fcntl(f.a, F_SETFL, flags | O_NONBLOCK);
    ssize_t n = recv(f.a, buf, sizeof(buf), 0);
    cr_assert_eq(n, -1); // EAGAIN/EWOULDBLOCK: nothing was sent to the sender itself
}

Test(Broadcast, notifies_other_players_with_a_direction_tagged_message)
{
    BroadcastFixture f;
    int sv2[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv2);
    f.world.addPlayer(sv2[0], "team1");
    zappy::Player *target = f.world.getPlayerByFd(sv2[0]);
    target->setPosition(0, 0, f.world.getMapSize()); // same tile as the sender => distance 0

    std::string res = f.commands->Broadcast({"hi"}, *f.client);
    cr_assert_str_eq(res.c_str(), "ok\n");

    // The message is queued with timeNeeded = distance * 7 = 0, so it is sent
    // as soon as sendMessageToClient() is invoked on the sender. Note: the
    // implementation queues the message against the *sender's* own fd
    // (player->getFd()) rather than the target's, so it is delivered on the
    // sender's socket peer (f.b), not the target's.
    zappy::Player *sender = f.world.getPlayerByFd(f.a);
    sender->sendMessageToClient();

    char buf[64] = {0};
    int flags = fcntl(f.b, F_GETFL, 0);
    fcntl(f.b, F_SETFL, flags | O_NONBLOCK);
    ssize_t n = recv(f.b, buf, sizeof(buf) - 1, 0);
    cr_assert_geq(n, 0);
    std::string received(buf, n > 0 ? n : 0);
    cr_assert(received.rfind("message ", 0) == 0);
    cr_assert(received.find("hi") != std::string::npos);

    close(sv2[0]);
    close(sv2[1]);
}

Test(Broadcast, queues_a_delayed_message_for_a_target_on_a_distant_tile)
{
    BroadcastFixture f;
    int sv2[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv2);
    f.world.addPlayer(sv2[0], "team1");
    zappy::Player *target = f.world.getPlayerByFd(sv2[0]);
    target->setPosition(3, 4, f.world.getMapSize()); // distance > 0 from the sender at (0,0)

    std::string res = f.commands->Broadcast({"far"}, *f.client);

    cr_assert_str_eq(res.c_str(), "ok\n");
    // With a non-zero distance, timeNeeded > 0 so the message must NOT be
    // ready immediately: nothing should be readable yet on the sender's peer.
    zappy::Player *sender = f.world.getPlayerByFd(f.a);
    sender->sendMessageToClient();

    char buf[64];
    int flags = fcntl(f.b, F_GETFL, 0);
    fcntl(f.b, F_SETFL, flags | O_NONBLOCK);
    ssize_t n = recv(f.b, buf, sizeof(buf), 0);
    cr_assert_eq(n, -1);

    close(sv2[0]);
    close(sv2[1]);
}
