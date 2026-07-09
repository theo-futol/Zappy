#include "World.hpp"
#include <algorithm>
#include <iostream>
#include <set>

namespace zappy
{

World::World(int x, int y, int f, bool useOldGen) : _f(f), _broadcastQueue(nullptr), _useOldGen(useOldGen)
{
    _map.resize(x);
    for (auto &column : _map)
        column.resize(y);

    for (int i = 0; i < x; ++i)
        for (int j = 0; j < y; ++j)
            _map[i][j] = tile();
    _mapSize = std::make_pair(x, y);
    srand(time(nullptr));
    resourcePassiveGeneration();
}

/// @brief Generates resources on the map at regular intervals.
/// Dispatches to the legacy or spec-accurate algorithm depending on -oldgen.
void World::resourcePassiveGeneration()
{
    if (_useOldGen)
        resourcePassiveGenerationLegacy();
    else
        resourcePassiveGenerationEven();
}

/// @brief Legacy algorithm, preserved as-is (including the deraumere density quirk)
/// for comparison/rollback via -oldgen. Via the formula : map_width * map_height * density
/// food: 0.5, linemate: 0.3, deraumere: 0.5, sibur: 0.1, mendiane: 0.1, phiras: 0.08, thystame: 0.05
void World::resourcePassiveGenerationLegacy()
{
    const std::vector<std::pair<ItemType, double>> resourceDensity = {{ItemType::FOOD, 0.5},     {ItemType::LINEMATE, 0.3}, {ItemType::DERAUMERE, 0.5}, {ItemType::SIBUR, 0.1},
                                                                      {ItemType::MENDIANE, 0.1}, {ItemType::PHIRAS, 0.08},  {ItemType::THYSTAME, 0.05}};
    int totalTiles = _map.size() * (_map.empty() ? 0 : _map[0].size());
    std::set<std::pair<int, int>> changedTiles;

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
            {
                it->second += 1;
                changedTiles.emplace(x, y);
            }
        }
    }
    for (const auto &[x, y] : changedTiles)
        broadcastTileContent({x, y});
}

/// @brief Spec-accurate algorithm: tops every resource up to map_width * map_height *
/// density (at least 1, so Trantor always has one of each on the floor), never
/// generating past the target nor when already there. Placement is purely random
/// (independent random tile per unit) - a given tile may end up with nothing.
/// food: 0.5, linemate: 0.3, deraumere: 0.15, sibur: 0.1, mendiane: 0.1,
/// phiras: 0.08, thystame: 0.05
void World::resourcePassiveGenerationEven()
{
    const std::vector<std::pair<ItemType, double>> resourceDensity = {{ItemType::FOOD, 0.5},     {ItemType::LINEMATE, 0.3}, {ItemType::DERAUMERE, 0.15}, {ItemType::SIBUR, 0.1},
                                                                      {ItemType::MENDIANE, 0.1}, {ItemType::PHIRAS, 0.08},  {ItemType::THYSTAME, 0.05}};
    if (_map.empty() || _map[0].empty())
        return;
    int width = static_cast<int>(_map.size());
    int height = static_cast<int>(_map[0].size());
    std::set<std::pair<int, int>> changedTiles;

    for (const auto &[type, density] : resourceDensity)
    {
        int target = std::max(1, static_cast<int>(width * height * density));
        int current = 0;
        for (const auto &column : _map)
            for (const auto &tileAt : column)
            {
                auto it = std::find_if(tileAt._items.begin(), tileAt._items.end(), [type](const std::pair<ItemType, int> &item) { return item.first == type; });
                if (it != tileAt._items.end())
                    current += it->second;
            }
        int toAdd = target - current;
        if (toAdd <= 0)
            continue;

        for (int i = 0; i < toAdd; ++i)
        {
            int x = rand() % width;
            int y = rand() % height;
            std::vector<std::pair<ItemType, int>> &tileItems = _map[x][y]._items;
            auto it = std::find_if(tileItems.begin(), tileItems.end(), [type](const std::pair<ItemType, int> &item) { return item.first == type; });
            if (it != tileItems.end())
            {
                it->second += 1;
                changedTiles.emplace(x, y);
            }
        }
    }
    for (const auto &[x, y] : changedTiles)
        broadcastTileContent({x, y});
}

std::vector<std::unique_ptr<Player>> &World::getPlayers()
{
    return _players;
}

const std::vector<std::unique_ptr<Player>> &World::getPlayers() const
{
    return _players;
}

Player *World::getPlayerById(int id)
{
    for (const auto &player : _players)
        if (player->getId() == id)
            return player.get();
    return nullptr;
}

Player *World::getPlayerById(int id) const
{
    for (const auto &player : _players)
        if (player->getId() == id)
            return player.get();
    return nullptr;
}

std::pair<int, int> World::getMapSize() const
{
    if (_map.empty())
        return std::make_pair(0, 0);
    return _mapSize;
}

int World::getTimeUnit() const
{
    return _f;
}

void World::setTimeUnit(int f)
{
    if (f < 1 || f > 1000)
        return;
    _f = f;
}

bool World::isPaused() const
{
    return _paused;
}

void World::setPaused(bool paused)
{
    _paused = paused;
}

tile *World::getTileAt(int playerID)
{
    Player *player = getPlayerById(playerID);
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

void World::broadcastTileContent(position pos)
{
    tile *tilePtr = getTileAt(pos);

    if (!_broadcastQueue || !tilePtr)
        return;
    std::string message = "bct " + std::to_string(pos.x) + " " + std::to_string(pos.y);
    for (const auto &item : tilePtr->_items)
        message += " " + std::to_string(item.second);
    message += "\n";
    _broadcastQueue->push(message);
}

void World::setTileAt(position pos, ItemType itemType, int count)
{
    tile *tilePtr = getTileAt(pos);

    if (!tilePtr)
    {
        Logger::log("8451", "World : tile update failed", {{"x", std::to_string(pos.x)}, {"y", std::to_string(pos.y)}});
        return;
    }
    std::vector<std::pair<ItemType, int>> &tileItems = tilePtr->_items;
    auto it = std::find_if(tileItems.begin(), tileItems.end(), [itemType](const std::pair<ItemType, int> &item) { return item.first == itemType; });

    if (it != tileItems.end())
        it->second = count;
    else
        tileItems.emplace_back(itemType, count);
    Logger::log("020", "World : resource spawned on tile", {{"item", itemTypeToString(itemType)}, {"x", std::to_string(pos.x)}, {"y", std::to_string(pos.y)}});
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
    auto team = std::make_shared<Team>(name, teamID, 0);
    for (int i = 0; i < initialSlots; ++i)
    {
        position eggPos{rand() % _mapSize.first, rand() % _mapSize.second};
        int eggId = team->addEgg(eggPos);
        if (_broadcastQueue)
            _broadcastQueue->push("enw " + std::to_string(eggId) + " -1 " + std::to_string(eggPos.x) + " " + std::to_string(eggPos.y) + "\n");
    }
    _teams.push_back(team);
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

int World::addPlayer(int fd, const std::string &teamName)
{
    std::shared_ptr<Team> team = getTeamByName(teamName);
    if (!team)
        return -1;
    std::vector<std::pair<position, int>> hatchable;
    for (const auto &eggGroup : team->getEggs())
        for (const Egg &egg : eggGroup.second)
            hatchable.emplace_back(eggGroup.first, egg.id);
    if (hatchable.empty())
        return -1; // no egg to hatch from: caller disconnects the client

    const auto &[spawn, eggId] = hatchable[rand() % hatchable.size()];
    team->removeEgg(spawn, 1, eggId);
    ++team->_slotsOccupied;
    if (_broadcastQueue)
        _broadcastQueue->push("ebo " + std::to_string(eggId) + "\n");

    int id = _nextPlayerId;
    _nextPlayerId++;
    _players.push_back(std::make_unique<Player>(id, fd, team));
    Player *player = _players.back().get();
    player->setPosition(spawn.x, spawn.y, _mapSize);
    std::vector<int> rotations = {0, 1, 2, 3};
    player->setRotation(rotations[rand() % rotations.size()]);
    addPlayerToTile(player, player->getPosition());
    return id;
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

void World::sendMessageToPlayersThatAreOnTile(position pos, const std::string &message, std::vector<std::unique_ptr<Client>> &clients)
{
    tile *tilePtr = getTileAt(pos);

    if (!tilePtr)
        return;
    std::vector<Client *> clientsOnTile;
    for (const auto &player : tilePtr->_players)
        for (const auto &client : clients)
            if (client->getPlayerId() == player->getId())
            {
                clientsOnTile.push_back(client.get());
                break;
            }
    for (Client *client : clientsOnTile)
        client->write(message);
}

void World::setBroadCastQueue(std::queue<std::string> *broadcastQueue)
{
    _broadcastQueue = broadcastQueue;
}

std::vector<int> World::foodCheck()
{
    std::vector<int> deadIds;

    for (const auto &player : _players)
    {
        if (player->getInventory().getItemCount(ItemType::FOOD) <= 0)
        {
            Logger::log("044", "Player : died", {{"player_id", std::to_string(player->getId())}});
            if (_broadcastQueue)
                _broadcastQueue->push("pdi " + std::to_string(player->getId()) + "\n");
            player->setState(PlayerState::DEAD);
            deadIds.push_back(player->getId());
        }
        else
            player->getInventory().removeItem(ItemType::FOOD);
    }
    for (int id : deadIds)
        removePlayer(id);
    return deadIds;
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
    int playersAtLevel = static_cast<int>(getPlayersOnTileAtLevel(x, y, level).size());
    if (playersAtLevel < requirement->players)
    {
        Logger::log("8422", "Incantation : failed, conditions not met", {{"x", std::to_string(x)}, {"y", std::to_string(y)}});
        return false;
    }
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
        {
            Logger::log("8422", "Incantation : failed, conditions not met", {{"x", std::to_string(x)}, {"y", std::to_string(y)}});
            return false;
        }
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
    broadcastTileContent({x, y});
}

Team *World::getWinningTeam() const
{
    for (const auto &team : _teams)
        if (team->_hasWin)
            return team.get();
    return nullptr;
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
                std::cout << "Team " << team->_name << " has won the game!" << std::endl;
                return true;
            }
        }
    }
    return false;
}

void World::removePlayer(int id)
{
    auto it = std::find_if(_players.begin(), _players.end(), [id](const std::unique_ptr<Player> &player) { return player->getId() == id; });

    if (it == _players.end())
        return;
    tile *tilePtr = getTileAt((*it)->getPosition());
    if (tilePtr && tilePtr->_incantationInProgress)
        tilePtr->_incantationInProgress = false;
    removePlayerFromTile(it->get(), (*it)->getPosition());
    // setState(DEAD) already frees the team slot (see Player::setState); avoid freeing it twice.
    if ((*it)->getState() != PlayerState::DEAD)
        (*it)->getTeam().removePlayer();
    _players.erase(it);
}

Map &World::getMap()
{
    return _map;
}

Map const &World::getMap() const
{
    return _map;
}

} // namespace zappy
