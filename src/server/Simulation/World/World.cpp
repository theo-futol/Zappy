#include "World.hpp"
#include <iostream>

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

std::vector<std::unique_ptr<Player>> &World::getPlayers()
{
    return _players;
}

const std::vector<std::unique_ptr<Player>> &World::getPlayers() const
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
    if (pos.x < 0 || pos.y < 0)
        return nullptr;
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
        if (team->_name == teamName)
            return team->getAvailableSlots();
    return -1;
}

void World::addTeam(const std::string &name, int teamID, int initialSlots)
{
    _teams.push_back(std::make_shared<Team>(name, teamID, initialSlots));
}

std::shared_ptr<Team> World::getTeamByName(const std::string &name)
{
    for (auto &team : _teams)
        if (team->_name == name)
            return team;
    return nullptr;
}

std::vector<std::shared_ptr<Team>> &World::getTeams()
{
    return _teams;
}

void World::addPlayer(int fd, const std::string &teamName)
{
    std::shared_ptr<Team> team = getTeamByName(teamName);
    if (!team)
        return;
    team->addPlayer();
    _players.push_back(std::make_unique<Player>(fd, team));
    addPlayerToTile(_players.back().get(), _players.back()->getPosition());
}

void World::removePlayerFromTile(Player *player, position pos)
{
    tile *tilePtr = getTileAt(pos);
    if (!tilePtr)
        return;
    tilePtr->_players.erase(std::remove(tilePtr->_players.begin(), tilePtr->_players.end(), player), tilePtr->_players.end());
}

void World::addPlayerToTile(Player *player, position pos)
{
    tile *tilePtr = getTileAt(pos);
    if (tilePtr)
        tilePtr->_players.push_back(player);
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

std::vector<int> World::foodCheck()
{
    std::vector<int> deadFds;

    for (const auto &player : _players)
    {
        if (player->getInventory().getItemCount(ItemType::FOOD) <= 0)
        {
            std::cout << "Player " << player->getFd() << " died from starvation at level " << player->getLevel() << std::endl;
            if (_broadcastQueue)
                _broadcastQueue->push("pdi " + std::to_string(player->getFd()) + "\n");
            player->setState(PlayerState::DEAD);
            deadFds.push_back(player->getFd());
        }
        else
            player->getInventory().removeItem(ItemType::FOOD);
    }
    for (int fd : deadFds)
        removePlayer(fd);
    return deadFds;
}

static const std::vector<ElevationRequirement> elevationRequirements = {
    {0, {}},
    {1, {{ItemType::LINEMATE, 1}}},
    {2, {{ItemType::LINEMATE, 1}, {ItemType::DERAUMERE, 1}, {ItemType::SIBUR, 1}}},
    {2, {{ItemType::LINEMATE, 2}, {ItemType::SIBUR, 1}, {ItemType::PHIRAS, 2}}},
    {4, {{ItemType::LINEMATE, 1}, {ItemType::DERAUMERE, 1}, {ItemType::SIBUR, 2}, {ItemType::PHIRAS, 1}}},
    {4, {{ItemType::LINEMATE, 1}, {ItemType::DERAUMERE, 2}, {ItemType::SIBUR, 1}, {ItemType::MENDIANE, 3}}},
    {6, {{ItemType::LINEMATE, 1}, {ItemType::DERAUMERE, 2}, {ItemType::SIBUR, 3}, {ItemType::PHIRAS, 1}}},
    {6, {{ItemType::LINEMATE, 2}, {ItemType::DERAUMERE, 2}, {ItemType::SIBUR, 2}, {ItemType::MENDIANE, 2}, {ItemType::PHIRAS, 2}, {ItemType::THYSTAME, 1}}},
};

const ElevationRequirement *World::getElevationRequirement(int level) const
{
    if (level < 1 || level > 7)
        return nullptr;
    return &elevationRequirements[level];
}

std::vector<Player *> World::getPlayersOnTileAtLevel(int x, int y, int level)
{
    std::vector<Player *> result;
    position pos{x, y};

    for (const auto &player : _players)
        if (player->getLevel() == level && player->getPosition() == pos)
            result.push_back(player.get());
    return result;
}

bool World::isIncantationValid(int x, int y, int level)
{
    const ElevationRequirement *requirement = getElevationRequirement(level);
    tile *tilePtr = getTileAt({x, y});

    if (!requirement || !tilePtr)
        return false;
    if (static_cast<int>(getPlayersOnTileAtLevel(x, y, level).size()) < requirement->players)
        return false;
    for (const auto &[stone, needed] : requirement->stones)
    {
        int available = 0;
        for (const auto &item : tilePtr->_items)
            if (item.first == stone)
            {
                available = item.second;
                break;
            }
        if (available < needed)
            return false;
    }
    return true;
}

void World::removeIncantationStones(int x, int y, int level)
{
    const ElevationRequirement *requirement = getElevationRequirement(level);
    tile *tilePtr = getTileAt({x, y});

    if (!requirement || !tilePtr)
        return;
    for (const auto &[stone, needed] : requirement->stones)
        for (auto &item : tilePtr->_items)
            if (item.first == stone)
            {
                item.second = std::max(0, item.second - needed);
                break;
            }
}

bool World::checkWinningCondition()
{
    for (auto &team : _teams)
    {
        int playerCounter = 0;
        if (team->_slotsOccupied >= 6)
        {
            for (auto &player : _players)
            {
                if (player->getTeam()._name != team->_name)
                    continue;
                if (player->getLevel() >= 8)
                    playerCounter++;
            }
            if (playerCounter >= 6)
            {
                _broadcastQueue->push("seg " + team->_name + "\n");
                team->_hasWin = true;
                return true;
            }
        }
    }
    return false;
}

void World::removePlayer(int fd)
{
    auto it = std::find_if(_players.begin(), _players.end(), [fd](const std::unique_ptr<Player> &player) { return player->getFd() == fd; });

    if (it == _players.end())
        return;
    removePlayerFromTile(it->get(), (*it)->getPosition());
    // setState(DEAD) already frees the team slot (see Player::setState); avoid freeing it twice.
    if ((*it)->getState() != PlayerState::DEAD)
        (*it)->getTeam().removePlayer();
    _players.erase(it);
}
} // namespace zappy
