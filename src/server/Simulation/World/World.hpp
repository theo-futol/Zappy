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

struct tile
{
    std::vector<std::shared_ptr<Player>> _players;
    std::vector<std::pair<ItemType, int>> _items;

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

using Map = std::vector<std::vector<tile>>;

class World
{
  private:
    std::vector<std::shared_ptr<Player>> _players;
    Map _map;
    std::pair<int, int> _mapSize;
    std::vector<Team> _teams;
    std::queue<std::string> *_broadcastQueue;


  public:
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
    void ressourcePassiveGeneration();
    bool checkWinningCondition();
    void foodCheck();
    std::vector<std::shared_ptr<Player>> &getPlayers();
    const std::vector<std::shared_ptr<Player>> &getPlayers() const;
    Player *getPlayerByFd(int fd);
    Player *getPlayerByFd(int fd) const;
    std::pair<int, int> getMapSize() const;
  void setBroadCastQueue(std::queue<std::string> *broadcastQueue);


    std::vector<Team>& getTeams();
    tile *getTileAt(int playerID);
    tile *getTileAt(position pos);
    void setTileAt(position pos, ItemType itemType, int count);
    int getAvailableSlotsForTeam(const std::string &teamName) const;
    void addTeam(const std::string &name, int teamID, int initialSlots);
    Team *getTeamByName(const std::string &name);
    void addPlayer(int fd, const std::string &teamName);
    void sendMessageToPlayersThatAreOnTile(position pos, const std::string &message);
};
} // namespace zappy
