#include "Network/Client/Client.hpp"
#include "Simulation/Player/Player.hpp"
#include "Simulation/Player/Teams.hpp"
#include "Simulation/Utils.hpp"
#include <chrono>
#include <criterion/criterion.h>
#include <fcntl.h>
#include <memory>
#include <sys/socket.h>
#include <unistd.h>

static std::shared_ptr<zappy::Team> mkTeam(const std::string &name = "t")
{
    return std::make_shared<zappy::Team>(name, 0, 0);
}

// Helper: open a socket pair and close it in tests that don't need the fd alive.
#define MAKE_SV(a, b)                                                                                                                                                              \
    int _sv[2];                                                                                                                                                                    \
    socketpair(AF_UNIX, SOCK_STREAM, 0, _sv);                                                                                                                                      \
    int a = _sv[0], b = _sv[1]

#define CLOSE_SV(a, b)                                                                                                                                                             \
    do                                                                                                                                                                             \
    {                                                                                                                                                                              \
        close(a);                                                                                                                                                                  \
        close(b);                                                                                                                                                                  \
    } while (0)

// ---- constructor / basic getters -----------------------------------------

Test(Player, constructor_sets_id_fd_and_defaults)
{
    MAKE_SV(a, b);
    auto team = mkTeam();
    zappy::Player p(42, a, team);
    cr_assert_eq(p.getId(), 42);
    cr_assert_eq(p.getFd(), a);
    cr_assert_eq(p.getLevel(), 1);
    cr_assert_eq(p.getState(), zappy::PlayerState::PENDING);
    CLOSE_SV(a, b);
}

Test(Player, getTeam_returns_correct_team)
{
    MAKE_SV(a, b);
    auto team = mkTeam("myteam");
    zappy::Player p(1, a, team);
    cr_assert_str_eq(p.getTeam()._name.c_str(), "myteam");
    CLOSE_SV(a, b);
}

Test(Player, const_getTeam_is_accessible)
{
    MAKE_SV(a, b);
    auto team = mkTeam("constteam");
    zappy::Player p(1, a, team);
    const zappy::Player &cp = p;
    cr_assert_str_eq(cp.getTeam()._name.c_str(), "constteam");
    CLOSE_SV(a, b);
}

Test(Player, getState_returns_alive_initially)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    cr_assert_eq(p.getState(), zappy::PlayerState::PENDING);
    CLOSE_SV(a, b);
}

// ---- setRotation / getOrientation ----------------------------------------

Test(Player, setRotation_north_gives_orientation_1)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setRotation(zappy::NORTH);
    cr_assert_eq(p.getRotation(), 0);
    cr_assert_eq(p.getOrientation(), 1);
    CLOSE_SV(a, b);
}

Test(Player, setRotation_east_gives_orientation_2)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setRotation(zappy::EAST);
    cr_assert_eq(p.getRotation(), 90);
    cr_assert_eq(p.getOrientation(), 2);
    CLOSE_SV(a, b);
}

Test(Player, setRotation_south_gives_orientation_3)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setRotation(zappy::SOUTH);
    cr_assert_eq(p.getRotation(), 180);
    cr_assert_eq(p.getOrientation(), 3);
    CLOSE_SV(a, b);
}

Test(Player, setRotation_west_gives_orientation_4)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setRotation(zappy::WEST);
    cr_assert_eq(p.getRotation(), 270);
    cr_assert_eq(p.getOrientation(), 4);
    CLOSE_SV(a, b);
}

Test(Player, setRotation_ignores_non_cardinal_value)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setRotation(45);
    cr_assert_eq(p.getRotation(), 0); // unchanged from default NORTH
    CLOSE_SV(a, b);
}

Test(Player, setRotation_ignores_negative_value)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setRotation(-90);
    cr_assert_eq(p.getRotation(), 0);
    CLOSE_SV(a, b);
}

Test(Player, setRotation_ignores_value_above_360)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setRotation(450);
    cr_assert_eq(p.getRotation(), 0);
    CLOSE_SV(a, b);
}

// ---- setPosition ---------------------------------------------------------

Test(Player, setPosition_wraps_negative_x)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setPosition(-1, 5, {10, 10});
    cr_assert_eq(p.getPosition().x, 9);
    cr_assert_eq(p.getPosition().y, 5);
    CLOSE_SV(a, b);
}

Test(Player, setPosition_wraps_negative_y)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setPosition(5, -1, {10, 10});
    cr_assert_eq(p.getPosition().y, 9);
    CLOSE_SV(a, b);
}

Test(Player, setPosition_wraps_x_at_map_width)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setPosition(10, 5, {10, 10});
    cr_assert_eq(p.getPosition().x, 0);
    CLOSE_SV(a, b);
}

Test(Player, setPosition_wraps_y_at_map_height)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setPosition(5, 10, {10, 10});
    cr_assert_eq(p.getPosition().y, 0);
    CLOSE_SV(a, b);
}

// ---- nextPosition --------------------------------------------------------

Test(Player, nextPosition_north_decrements_y)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setPosition(5, 5, {10, 10});
    p.setRotation(zappy::NORTH);
    auto next = p.nextPosition({10, 10});
    cr_assert_eq(next.x, 5);
    cr_assert_eq(next.y, 4);
    CLOSE_SV(a, b);
}

Test(Player, nextPosition_east_increments_x)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setPosition(5, 5, {10, 10});
    p.setRotation(zappy::EAST);
    auto next = p.nextPosition({10, 10});
    cr_assert_eq(next.x, 6);
    cr_assert_eq(next.y, 5);
    CLOSE_SV(a, b);
}

Test(Player, nextPosition_south_increments_y)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setPosition(5, 5, {10, 10});
    p.setRotation(zappy::SOUTH);
    auto next = p.nextPosition({10, 10});
    cr_assert_eq(next.x, 5);
    cr_assert_eq(next.y, 6);
    CLOSE_SV(a, b);
}

Test(Player, nextPosition_west_decrements_x)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setPosition(5, 5, {10, 10});
    p.setRotation(zappy::WEST);
    auto next = p.nextPosition({10, 10});
    cr_assert_eq(next.x, 4);
    cr_assert_eq(next.y, 5);
    CLOSE_SV(a, b);
}

Test(Player, nextPosition_wraps_north_at_top)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setPosition(5, 0, {10, 10});
    p.setRotation(zappy::NORTH);
    auto next = p.nextPosition({10, 10});
    cr_assert_eq(next.y, 9);
    CLOSE_SV(a, b);
}

Test(Player, nextPosition_wraps_west_at_left)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setPosition(0, 5, {10, 10});
    p.setRotation(zappy::WEST);
    auto next = p.nextPosition({10, 10});
    cr_assert_eq(next.x, 9);
    CLOSE_SV(a, b);
}

Test(Player, nextPosition_wraps_east_at_right)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setPosition(9, 5, {10, 10});
    p.setRotation(zappy::EAST);
    auto next = p.nextPosition({10, 10});
    cr_assert_eq(next.x, 0);
    CLOSE_SV(a, b);
}

Test(Player, nextPosition_wraps_south_at_bottom)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setPosition(5, 9, {10, 10});
    p.setRotation(zappy::SOUTH);
    auto next = p.nextPosition({10, 10});
    cr_assert_eq(next.y, 0);
    CLOSE_SV(a, b);
}

// ---- levelUp -------------------------------------------------------------

Test(Player, levelUp_increments_level)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    cr_assert_eq(p.getLevel(), 1);
    p.levelUp();
    cr_assert_eq(p.getLevel(), 2);
    p.levelUp();
    cr_assert_eq(p.getLevel(), 3);
    CLOSE_SV(a, b);
}

// ---- setState ------------------------------------------------------------

Test(Player, setState_alive_to_dead_frees_team_slot)
{
    MAKE_SV(a, b);
    auto team = mkTeam();
    team->_slotsOccupied = 2;
    zappy::Player p(1, a, team);
    p.setState(zappy::PlayerState::DEAD);
    cr_assert_eq(p.getState(), zappy::PlayerState::DEAD);
    cr_assert_eq(team->_slotsOccupied, 1);
    CLOSE_SV(a, b);
}

Test(Player, setState_dead_twice_does_not_double_free)
{
    MAKE_SV(a, b);
    auto team = mkTeam();
    team->_slotsOccupied = 1;
    zappy::Player p(1, a, team);
    p.setState(zappy::PlayerState::DEAD);
    p.setState(zappy::PlayerState::DEAD);
    cr_assert_eq(team->_slotsOccupied, 0);
    CLOSE_SV(a, b);
}

// ---- freeze --------------------------------------------------------------

Test(Player, isFrozen_false_initially)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    cr_assert_not(p.isFrozen());
    CLOSE_SV(a, b);
}

Test(Player, setFrozenUntil_future_makes_player_frozen)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setFrozenUntil(std::chrono::steady_clock::now() + std::chrono::seconds(10));
    cr_assert(p.isFrozen());
    CLOSE_SV(a, b);
}

Test(Player, setFrozenUntil_past_does_not_freeze)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setFrozenUntil(std::chrono::steady_clock::now() - std::chrono::milliseconds(1));
    cr_assert_not(p.isFrozen());
    CLOSE_SV(a, b);
}

// ---- getDistanceTo -------------------------------------------------------

Test(Player, getDistanceTo_position_pythagorean)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setPosition(0, 0, {20, 20});
    int d = p.getDistanceTo({3, 4}, {20, 20});
    cr_assert_eq(d, 5); // sqrt(9+16)=5
    CLOSE_SV(a, b);
}

Test(Player, getDistanceTo_position_wraps_x)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setPosition(0, 0, {10, 10});
    // dx=9 normally but wrap -> dx=1; dy=0 -> d=1
    int d = p.getDistanceTo({9, 0}, {10, 10});
    cr_assert_eq(d, 1);
    CLOSE_SV(a, b);
}

Test(Player, getDistanceTo_position_wraps_y)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.setPosition(0, 0, {10, 10});
    // dy=9 normally but wrap -> dy=1; dx=0 -> d=1
    int d = p.getDistanceTo({0, 9}, {10, 10});
    cr_assert_eq(d, 1);
    CLOSE_SV(a, b);
}

Test(Player, getDistanceTo_player_overload)
{
    int sv1[2], sv2[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv1);
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv2);
    auto team = mkTeam();
    zappy::Player p1(1, sv1[0], team);
    zappy::Player p2(2, sv2[0], team);
    p1.setPosition(0, 0, {20, 20});
    p2.setPosition(3, 4, {20, 20});
    cr_assert_eq(p1.getDistanceTo(p2, {20, 20}), 5);
    close(sv1[0]);
    close(sv1[1]);
    close(sv2[0]);
    close(sv2[1]);
}

// ---- addMessageToQueue + sendMessageToClient ----------------------------

Test(Player, addMessageToQueue_and_sendMessageToClient_delivers_immediately)
{
    // sv: player fd pair (Player holds sv[0]; we use sv[1] only to keep the pair open)
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    // recv pair: Client wraps rc[0], test reads from rc[1]
    int rc[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, rc);
    int flags = fcntl(rc[1], F_GETFL, 0);
    fcntl(rc[1], F_SETFL, flags | O_NONBLOCK);

    zappy::Player p(1, sv[0], mkTeam());
    p.addMessageToQueue("hello\n", 0, rc[0]); // timeNeeded=0 → immediate

    std::vector<std::unique_ptr<zappy::Client>> clients;
    clients.push_back(std::make_unique<zappy::Client>(rc[0]));
    p.sendMessageToClient(std::clock(), clients);
    clients.clear(); // Client dtor closes rc[0]

    char buf[64] = {0};
    ssize_t n = recv(rc[1], buf, sizeof(buf) - 1, 0);
    cr_assert_geq(n, 0);
    cr_assert_str_eq(buf, "hello\n");

    close(sv[0]);
    close(sv[1]);
    close(rc[1]);
}

Test(Player, addMessageToQueue_groups_same_message)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.addMessageToQueue("msg\n", 10, b);
    p.addMessageToQueue("msg\n", 20, b); // same text → appended to existing entry
    // No crash = success
    CLOSE_SV(a, b);
}

Test(Player, addMessageToQueue_creates_separate_entry_for_different_message)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.addMessageToQueue("msg1\n", 10, b);
    p.addMessageToQueue("msg2\n", 20, b);
    // No crash = success
    CLOSE_SV(a, b);
}

Test(Player, sortQueueByTimeNeeded_does_not_crash)
{
    MAKE_SV(a, b);
    zappy::Player p(1, a, mkTeam());
    p.addMessageToQueue("x\n", 100, b);
    p.addMessageToQueue("x\n", 10, b);
    p.sortQueueByTimeNeeded();
    CLOSE_SV(a, b);
}

Test(Player, sendMessageToClient_skips_not_yet_ready_messages)
{
    MAKE_SV(a, b);
    int rc[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, rc);
    int flags = fcntl(rc[1], F_GETFL, 0);
    fcntl(rc[1], F_SETFL, flags | O_NONBLOCK);

    zappy::Player p(1, a, mkTeam());
    // timeNeeded = 1 000 000 ms → not ready for a very long time
    p.addMessageToQueue("late\n", 1000000, rc[0]);

    std::vector<std::unique_ptr<zappy::Client>> clients;
    clients.push_back(std::make_unique<zappy::Client>(rc[0]));
    p.sendMessageToClient(std::clock(), clients);
    clients.clear();

    char buf[64] = {0};
    ssize_t n = recv(rc[1], buf, sizeof(buf) - 1, 0);
    // clients.clear() already closed rc[0] → rc[1] sees EOF (0) or EAGAIN (-1),
    // but NOT the deferred message "late\n".
    cr_assert_leq(n, 0);

    close(a);
    close(b);
    close(rc[1]);
}
