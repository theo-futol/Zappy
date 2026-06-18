#include "World.hpp"

namespace zappy
{

World::World(int x, int y) : _broadcastQueue(nullptr)
{
    _map.resize(x);
    for (auto &column : _map)
        column.resize(y);

    for (int i = 0; i < x; ++i)
        for (int j = 0; j < y; ++j)
            _map[i][j] = tile();
    _mapSize = std::make_pair(x, y);
    srand(time(nullptr));
}

/// @brief Generates resources on the map at regular intervals.
/// Via the formula : map_width * map_height * density
/// @note The density and generation logic can be adjusted based on game requirements.
/// This function should be called periodically, e.g., every 20 seconds, to simulate resource regeneration.
/// The density of each ressource are the following:
/// food: 0.5, linemate: 0.3, deraumere: 0.5, sibur: 0.1, mendiane: 0.1, phiras: 0.08, thystame: 0.05
void World::ressourcePassiveGeneration()
{
    const std::vector<std::pair<ItemType, double>> resourceDensity = {{ItemType::FOOD, 0.5},     {ItemType::LINEMATE, 0.3}, {ItemType::DERAUMERE, 0.5}, {ItemType::SIBUR, 0.1},
                                                                      {ItemType::MENDIANE, 0.1}, {ItemType::PHIRAS, 0.08},  {ItemType::THYSTAME, 0.05}};
    int totalTiles = _map.size() * (_map.empty() ? 0 : _map[0].size());

    for (const auto &[type, density] : resourceDensity)
    {
        int totalToGenerate = static_cast<int>(totalTiles * density);
        for (int i = 0; i < totalToGenerate; ++i)
        {
            int x = rand() % _map.size();
            int y = rand() % (_map.empty() ? 1 : _map[0].size());
            std::vector<std::pair<zappy::ItemType, int>> &tileCoords = _map[x][y]._items;
            auto it = std::find_if(tileCoords.begin(), tileCoords.end(), [type](const std::pair<ItemType, int> &item) { return item.first == type; });
            if (it != tileCoords.end())
                it->second += 1;
        }
    }
}

std::vector<std::shared_ptr<Player>> &World::getPlayers()
{
    return _players;
}

const std::vector<std::shared_ptr<Player>> &World::getPlayers() const
{
    return _players;
}

Player *World::getPlayerByFd(int fd)
{
    for (const auto &player : _players)
        if (player->getFd() == fd)
            return player.get();
    return nullptr;
}

Player *World::getPlayerByFd(int fd) const
{
    for (const auto &player : _players)
        if (player->getFd() == fd)
            return player.get();
    return nullptr;
}

std::pair<int, int> World::getMapSize() const
{
    if (_map.empty())
        return std::make_pair(0, 0);
    return _mapSize;
}

tile *World::getTileAt(int playerID)
{
    Player *player = getPlayerByFd(playerID);
    if (!player)
        return nullptr;
    return getTileAt(player->getPosition());
}

tile *World::getTileAt(position pos)
{
    if (pos.x >= static_cast<int>(_map.size()) || pos.y >= static_cast<int>(_map[0].size()))
        return nullptr;
    return &_map[pos.x][pos.y];
}

void World::setTileAt(position pos, ItemType itemType, int count)
{
    tile *tilePtr = getTileAt(pos);

    if (!tilePtr)
        return;
    std::vector<std::pair<ItemType, int>> &tileItems = tilePtr->_items;
    auto it = std::find_if(tileItems.begin(), tileItems.end(), [itemType](const std::pair<ItemType, int> &item) { return item.first == itemType; });

    if (it != tileItems.end())
        it->second = count;
    else
        tileItems.emplace_back(itemType, count);
}

int World::getAvailableSlotsForTeam(const std::string &teamName) const
{
    for (const auto &team : _teams)
        if (team._name == teamName)
            return team.getAvailableSlots();
    return -1;
}

void World::addTeam(const std::string &name, int teamID, int initialSlots)
{
    _teams.emplace_back(name, teamID, initialSlots);
}

Team *World::getTeamByName(const std::string &name)
{
    for (auto &team : _teams)
        if (team._name == name)
            return &team;
    return nullptr;
}

std::vector<Team> &World::getTeams()
{
    return _teams;
}

void World::addPlayer(int fd, const std::string &teamName)
{
    Team *team = getTeamByName(teamName);
    if (!team)
        return;
    auto player = std::make_shared<Player>(fd, *team);
    _players.push_back(player);
}

void World::sendMessageToPlayersThatAreOnTile(position pos, const std::string &message)
{
    tile *tilePtr = getTileAt(pos);

    if (!tilePtr)
        return;
    for (const auto &player : tilePtr->_players)
        player->writeToClient(message);
}

void World::setBroadCastQueue(std::queue<std::string> *broadcastQueue)
{
    _broadcastQueue = broadcastQueue;
}

void World::foodCheck()
{
    for (auto player = _players.begin(); player != _players.end();)
    {
        if (player->get()->getInventory().getItemCount(ItemType::FOOD) <= 0)
        {
            _broadcastQueue->push("pdi " + std::to_string(player->get()->getFd()) + "\n");
            player->get()->setState(PlayerState::DEAD);
            player = _players.erase(player);
        }
        else
        {
            player->get()->getInventory().removeItem(ItemType::FOOD);
            ++player;
        }
    }
}

void World::checkWinningCondition()
{
    int playerCounter = 0;

    for (const auto &team : _teams)
        if (team._slotsOccupied >= 6)
        {
            for (auto &player : _players)
            {
                if (player->getTeam()._name != team._name)
                    continue;
                if (player->getLevel() >= 8)
                    playerCounter++;
            }
            if (playerCounter >= 6)
            {
                _broadcastQueue->push("seg " + team._name + "\n");
                break; // ADD SOMETHING ELSE TO STOP THE SERVER, UPDATE THE STATIC TOO
            }
        }
}
} // namespace zappy
