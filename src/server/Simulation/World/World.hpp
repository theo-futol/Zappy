#pragma once
#include "../Player/Player.hpp"
#include "Items.hpp"
#include <algorithm>
#include <memory>
#include <vector>

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

using Map = std::vector<std::vector<tile>>;

class World
{
  private:
    std::vector<std::shared_ptr<Player>> _players;
    Map _map;
    std::pair<int, int> _mapSize;

  private:
    std::vector<Team> _teams;

  public:
    World(int x, int y);
    ~World() = default;

    void ressourcePassiveGeneration();
    std::vector<std::shared_ptr<Player>> &getPlayers();
    const std::vector<std::shared_ptr<Player>> &getPlayers() const;
    Player *getPlayerByFd(int fd);
    Player *getPlayerByFd(int fd) const;
    std::pair<int, int> getMapSize() const;

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
