#pragma once
#include <algorithm>
#include <memory>
#include <vector>
#include <queue>
#include <thread>
#include <utility>

#include "../Player/Player.hpp"
#include "Items.hpp"

namespace zappy
{

/// @brief A single map cell: who is standing on it and what resources lie there.
struct tile
{
    std::vector<Player *> _players; // non-owning: players are owned by World::_players
    std::vector<std::pair<ItemType, int>> _items;
    bool _incantationInProgress = false; // true while an elevation ritual is running on this tile

    /// @brief Builds an empty tile pre-seeded with a zero count for each resource type.
    tile() : _players{}, _items{}
    {
        for (int i = static_cast<int>(ItemType::FOOD); i <= static_cast<int>(ItemType::THYSTAME); ++i)
            _items.emplace_back(static_cast<ItemType>(i), 0);
    }
};

/// @brief Requirements to elevate from a given level to the next one.
/// @note `players` is the minimum number of players of the same level that must
/// stand on the tile; `stones` lists the minimum count of each required stone.
struct ElevationRequirement
{
    int players;
    std::vector<std::pair<ItemType, int>> stones;
};

/// @brief The map: a 2D grid of tiles indexed [y][x].
using Map = std::vector<std::vector<tile>>;

/// @brief The authoritative game state: the map, the players, the teams, and the
///        simulation rules that act on them (resource generation, food/starvation,
///        elevation requirements and win condition).
class World
{
  private:
    std::vector<std::unique_ptr<Player>> _players;
    Map _map;
    std::pair<int, int> _mapSize;
    std::vector<std::shared_ptr<Team>> _teams;
    std::queue<std::string> *_broadcastQueue;


  public:
    /// @brief Builds an x-by-y world with empty tiles and no players yet.
    World(int x, int y);
    ~World() = default;

    /// @brief Checks whether an elevation from `level` to `level + 1` can take place on the tile.
    /// Verifies both the minimum number of same-level players and the required stones.
    bool isIncantationValid(int x, int y, int level);
    /// @brief Returns the requirements to elevate from `level` to `level + 1`, or nullptr if `level` is out of [1, 7].
    const ElevationRequirement *getElevationRequirement(int level) const;
    /// @brief Returns the players standing on the tile that are exactly at `level`.
    std::vector<Player *> getPlayersOnTileAtLevel(int x, int y, int level);
    /// @brief Removes from the tile the stones consumed by an elevation from `level` to `level + 1`.
    void removeIncantationStones(int x, int y, int level);

    /// @brief Periodically replenishes resources across the map (the spawn-rate tick).
    void ressourcePassiveGeneration();

    /// @brief Returns true once a team has reached the win condition (6 players lvl 8).
    bool checkWinningCondition();

    /// @brief Consumes one food unit per player and kills those who have starved.
    /// @return The fds of the players that died of starvation this tick.
    std::vector<int> foodCheck();

    /// @brief All players currently in the world.
    std::vector<std::unique_ptr<Player>> &getPlayers();
    const std::vector<std::unique_ptr<Player>> &getPlayers() const;

    /// @brief Looks up a player by its client fd, or nullptr if none matches.
    Player *getPlayerByFd(int fd);
    Player *getPlayerByFd(int fd) const;

    /// @brief Map dimensions as (width, height).
    std::pair<int, int> getMapSize() const;

    /// @brief Sets the queue used to push world events out to graphic clients.
    void setBroadCastQueue(std::queue<std::string> *broadcastQueue);


    /// @brief All teams in the game.
    std::vector<std::shared_ptr<Team>>& getTeams();

    /// @brief Returns the tile the given player is standing on.
    tile *getTileAt(int playerID);
    /// @brief Returns the tile at the given coordinates.
    tile *getTileAt(position pos);

    /// @brief Sets the absolute count of a resource on a tile.
    void setTileAt(position pos, ItemType itemType, int count);

    /// @brief Free connection slots remaining for a team (used to admit new players).
    int getAvailableSlotsForTeam(const std::string &teamName) const;

    /// @brief Registers a new team with an initial number of slots.
    void addTeam(const std::string &name, int teamID, int initialSlots);

    /// @brief Looks up a team by name, or nullptr if it does not exist.
    std::shared_ptr<Team> getTeamByName(const std::string &name);

    /// @brief Creates a player on the named team and places it in the world.
    void addPlayer(int fd, const std::string &teamName);

    /// @brief Removes the player pointer from the tile at the given position, if any.
    void removePlayerFromTile(Player *player, position pos);
    /// @brief Adds the player pointer to the tile at the given position, if it exists.
    void addPlayerToTile(Player *player, position pos);

    /// @brief Sends a message to every player currently standing on the given tile.
    void sendMessageToPlayersThatAreOnTile(position pos, const std::string &message);

    /// @brief Removes a player from the world and decrease the number of slots occupied in its team. The player is removed from the tile it was standing on and from the list of players in the world.
    void removePlayer(int fd);
};
} // namespace zappy
