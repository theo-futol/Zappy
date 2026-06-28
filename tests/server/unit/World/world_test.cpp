#include "Network/Client/Client.hpp"
#include "Simulation/Player/Teams.hpp"
#include "Simulation/World/World.hpp"
#include <criterion/criterion.h>
#include <memory>
#include <queue>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

// ---- construction / map --------------------------------------------------

Test(World, getMapSize_returns_correct_dimensions)
{
    zappy::World w(5, 7, 100);
    auto sz = w.getMapSize();
    cr_assert_eq(sz.first, 5);
    cr_assert_eq(sz.second, 7);
}

Test(World, getTileAt_returns_non_null_for_valid_position)
{
    zappy::World w(5, 5, 100);
    cr_assert_not_null(w.getTileAt({0, 0}));
    cr_assert_not_null(w.getTileAt({4, 4}));
}

Test(World, getTileAt_returns_null_for_out_of_bounds)
{
    zappy::World w(5, 5, 100);
    cr_assert_null(w.getTileAt({-1, 0}));
    cr_assert_null(w.getTileAt({0, -1}));
    cr_assert_null(w.getTileAt({5, 0}));
    cr_assert_null(w.getTileAt({0, 5}));
}

Test(World, getTileAt_by_player_id_returns_null_for_unknown_player)
{
    zappy::World w(5, 5, 100);
    cr_assert_null(w.getTileAt(9999));
}

Test(World, getTileAt_by_player_id_returns_tile_for_known_player)
{
    zappy::World w(5, 5, 100);
    w.addTeam("t", 0, 1);
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    int id = w.addPlayer(sv[0], "t");
    cr_assert_not_null(w.getTileAt(id));
    close(sv[0]);
    close(sv[1]);
}

// ---- teams ---------------------------------------------------------------

Test(World, addTeam_and_getTeamByName)
{
    zappy::World w(5, 5, 100);
    w.addTeam("heroes", 0, 3);
    auto team = w.getTeamByName("heroes");
    cr_assert_not_null(team.get());
    cr_assert_str_eq(team->_name.c_str(), "heroes");
}

Test(World, getTeamByName_returns_null_for_unknown_name)
{
    zappy::World w(5, 5, 100);
    cr_assert_null(w.getTeamByName("nobody").get());
}

Test(World, getAvailableSlotsForTeam_returns_correct_count)
{
    zappy::World w(5, 5, 100);
    w.addTeam("a", 0, 4);
    cr_assert_eq(w.getAvailableSlotsForTeam("a"), 4);
}

Test(World, getAvailableSlotsForTeam_returns_minus_one_for_unknown)
{
    zappy::World w(5, 5, 100);
    cr_assert_eq(w.getAvailableSlotsForTeam("ghost"), -1);
}

Test(World, getTeams_returns_all_teams)
{
    zappy::World w(5, 5, 100);
    w.addTeam("a", 0, 1);
    w.addTeam("b", 1, 1);
    cr_assert_eq(w.getTeams().size(), 2u);
}

// ---- players -------------------------------------------------------------

Test(World, addPlayer_returns_valid_id)
{
    zappy::World w(5, 5, 100);
    w.addTeam("t", 0, 2);
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    int id = w.addPlayer(sv[0], "t");
    cr_assert_geq(id, 1);
    close(sv[0]);
    close(sv[1]);
}

Test(World, addPlayer_returns_minus_one_for_unknown_team)
{
    zappy::World w(5, 5, 100);
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    int id = w.addPlayer(sv[0], "nobody");
    cr_assert_eq(id, -1);
    close(sv[0]);
    close(sv[1]);
}

Test(World, addPlayer_returns_minus_one_when_no_eggs_left)
{
    zappy::World w(5, 5, 100);
    w.addTeam("t", 0, 0); // 0 eggs
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    int id = w.addPlayer(sv[0], "t");
    cr_assert_eq(id, -1);
    close(sv[0]);
    close(sv[1]);
}

Test(World, getPlayerById_returns_player)
{
    zappy::World w(5, 5, 100);
    w.addTeam("t", 0, 1);
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    int id = w.addPlayer(sv[0], "t");
    cr_assert_not_null(w.getPlayerById(id));
    cr_assert_eq(w.getPlayerById(id)->getId(), id);
    close(sv[0]);
    close(sv[1]);
}

Test(World, getPlayerById_returns_null_for_unknown_id)
{
    zappy::World w(5, 5, 100);
    cr_assert_null(w.getPlayerById(9999));
}

Test(World, const_getPlayerById_returns_null_for_unknown_id)
{
    const zappy::World w(5, 5, 100);
    cr_assert_null(w.getPlayerById(9999));
}

Test(World, const_getPlayers_returns_all_players)
{
    zappy::World w(5, 5, 100);
    w.addTeam("t", 0, 2);
    int sv1[2], sv2[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv1);
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv2);
    w.addPlayer(sv1[0], "t");
    w.addPlayer(sv2[0], "t");
    const zappy::World &cw = w;
    cr_assert_eq(cw.getPlayers().size(), 2u);
    close(sv1[0]);
    close(sv1[1]);
    close(sv2[0]);
    close(sv2[1]);
}

// ---- setTileAt -----------------------------------------------------------

Test(World, setTileAt_updates_resource_count)
{
    zappy::World w(5, 5, 100);
    w.setTileAt({0, 0}, zappy::ItemType::LINEMATE, 7);
    zappy::tile *t = w.getTileAt({0, 0});
    bool found = false;
    for (const auto &item : t->_items)
        if (item.first == zappy::ItemType::LINEMATE && item.second == 7)
            found = true;
    cr_assert(found);
}

Test(World, setTileAt_out_of_bounds_does_not_crash)
{
    zappy::World w(5, 5, 100);
    w.setTileAt({-1, 0}, zappy::ItemType::FOOD, 1); // must not crash
}

// ---- time unit -----------------------------------------------------------

Test(World, getTimeUnit_returns_construction_value)
{
    zappy::World w(5, 5, 42);
    cr_assert_eq(w.getTimeUnit(), 42);
}

Test(World, setTimeUnit_updates_value)
{
    zappy::World w(5, 5, 100);
    w.setTimeUnit(500);
    cr_assert_eq(w.getTimeUnit(), 500);
}

Test(World, setTimeUnit_ignores_zero)
{
    zappy::World w(5, 5, 100);
    w.setTimeUnit(0);
    cr_assert_eq(w.getTimeUnit(), 100);
}

Test(World, setTimeUnit_ignores_above_1000)
{
    zappy::World w(5, 5, 100);
    w.setTimeUnit(1001);
    cr_assert_eq(w.getTimeUnit(), 100);
}

// ---- removePlayer --------------------------------------------------------

Test(World, removePlayer_removes_player_from_world)
{
    zappy::World w(5, 5, 100);
    w.addTeam("t", 0, 1);
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    int id = w.addPlayer(sv[0], "t");
    cr_assert_not_null(w.getPlayerById(id));
    w.removePlayer(id);
    cr_assert_null(w.getPlayerById(id));
    close(sv[0]);
    close(sv[1]);
}

Test(World, removePlayer_unknown_id_does_not_crash)
{
    zappy::World w(5, 5, 100);
    w.removePlayer(9999); // must not crash
}

Test(World, removePlayer_does_not_free_slot_twice_for_dead_player)
{
    zappy::World w(5, 5, 100);
    w.addTeam("t", 0, 1);
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    int id = w.addPlayer(sv[0], "t");
    zappy::Player *p = w.getPlayerById(id);
    p->setState(zappy::PlayerState::DEAD); // already frees the slot
    int slotsBefore = static_cast<int>(w.getTeamByName("t")->_slotsOccupied);
    w.removePlayer(id);
    // Slot should not be decremented again
    cr_assert_eq(static_cast<int>(w.getTeamByName("t")->_slotsOccupied), slotsBefore);
    close(sv[0]);
    close(sv[1]);
}

// ---- foodCheck -----------------------------------------------------------

Test(World, foodCheck_returns_empty_when_all_players_have_food)
{
    zappy::World w(5, 5, 100);
    w.addTeam("t", 0, 1);
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    int id = w.addPlayer(sv[0], "t");
    // Give the player food so it survives
    w.getPlayerById(id)->getInventory().addItem(zappy::ItemType::FOOD, 5);
    auto dead = w.foodCheck();
    cr_assert(dead.empty());
    close(sv[0]);
    close(sv[1]);
}

Test(World, foodCheck_kills_player_with_no_food)
{
    zappy::World w(5, 5, 100);
    std::queue<std::string> bq;
    w.setBroadCastQueue(&bq);
    w.addTeam("t", 0, 1);
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    int id = w.addPlayer(sv[0], "t");
    // Remove all food
    auto *player = w.getPlayerById(id);
    while (player->getInventory().getItemCount(zappy::ItemType::FOOD) > 0)
        player->getInventory().removeItem(zappy::ItemType::FOOD);
    auto dead = w.foodCheck();
    cr_assert_eq(dead.size(), 1u);
    cr_assert_eq(dead[0], id);
    // Player should have been removed
    cr_assert_null(w.getPlayerById(id));
    close(sv[0]);
    close(sv[1]);
}

// ---- setBroadCastQueue ---------------------------------------------------

Test(World, setBroadCastQueue_enables_tile_broadcasts)
{
    zappy::World w(5, 5, 100);
    std::queue<std::string> bq;
    w.setBroadCastQueue(&bq);
    w.setTileAt({0, 0}, zappy::ItemType::FOOD, 3);
    // setTileAt logs; broadcastTileContent only fires if a bq is set
    // and is called internally — just verify no crash
}

// ---- elevation -----------------------------------------------------------

Test(World, getElevationRequirement_returns_null_for_invalid_level)
{
    zappy::World w(5, 5, 100);
    cr_assert_null(w.getElevationRequirement(0));
    cr_assert_null(w.getElevationRequirement(8));
}

Test(World, getElevationRequirement_returns_non_null_for_valid_levels)
{
    zappy::World w(5, 5, 100);
    for (int lvl = 1; lvl <= 7; ++lvl)
        cr_assert_not_null(w.getElevationRequirement(lvl));
}

Test(World, isIncantationValid_false_for_level_0)
{
    zappy::World w(5, 5, 100);
    cr_assert_not(w.isIncantationValid(0, 0, 0));
}

Test(World, getPlayersOnTileAtLevel_returns_matching_players)
{
    zappy::World w(5, 5, 100);
    w.addTeam("t", 0, 2);
    int sv1[2], sv2[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv1);
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv2);
    w.addPlayer(sv1[0], "t");
    w.addPlayer(sv2[0], "t");
    // Both players are level 1; get them on the same tile by position
    auto players = w.getPlayersOnTileAtLevel(w.getPlayers()[0]->getPosition().x, w.getPlayers()[0]->getPosition().y, 1);
    cr_assert_geq(players.size(), 1u);
    close(sv1[0]);
    close(sv1[1]);
    close(sv2[0]);
    close(sv2[1]);
}

// ---- winning condition ---------------------------------------------------

Test(World, checkWinningCondition_false_when_no_team_has_6_level8_players)
{
    zappy::World w(5, 5, 100);
    std::queue<std::string> bq;
    w.setBroadCastQueue(&bq);
    w.addTeam("t", 0, 1);
    cr_assert_not(w.checkWinningCondition());
}

Test(World, getWinningTeam_returns_null_initially)
{
    zappy::World w(5, 5, 100);
    w.addTeam("t", 0, 1);
    cr_assert_null(w.getWinningTeam());
}

// ---- sendMessageToPlayersThatAreOnTile -----------------------------------

Test(World, sendMessageToPlayersThatAreOnTile_does_not_crash_on_empty_tile)
{
    zappy::World w(5, 5, 100);
    std::vector<std::unique_ptr<zappy::Client>> clients;
    w.sendMessageToPlayersThatAreOnTile({0, 0}, "msg\n", clients);
}

Test(World, sendMessageToPlayersThatAreOnTile_does_not_crash_out_of_bounds)
{
    zappy::World w(5, 5, 100);
    std::vector<std::unique_ptr<zappy::Client>> clients;
    w.sendMessageToPlayersThatAreOnTile({-1, -1}, "msg\n", clients);
}

// ---- legacy resource generation ------------------------------------------

Test(World, legacy_resource_generation_does_not_crash)
{
    // -oldGen flag triggers resourcePassiveGenerationLegacy; verify no crash.
    zappy::World w(5, 5, 100, true);
    (void)w;
}

// ---- addPlayerToTile / removePlayerFromTile ------------------------------

Test(World, addPlayerToTile_and_removePlayerFromTile_do_not_crash_on_null_tile)
{
    zappy::World w(5, 5, 100);
    w.addTeam("t", 0, 1);
    int sv[2];
    socketpair(AF_UNIX, SOCK_STREAM, 0, sv);
    int id = w.addPlayer(sv[0], "t");
    zappy::Player *p = w.getPlayerById(id);
    // Out-of-bounds position should just be a no-op
    w.removePlayerFromTile(p, {-1, -1});
    w.addPlayerToTile(p, {-1, -1});
    close(sv[0]);
    close(sv[1]);
}
