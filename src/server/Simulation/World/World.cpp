#include "World.hpp"

namespace zappy
{
World::World(int x, int y)
{
    _map.resize(x);
    for (auto &column : _map)
        column.resize(y);

    for (int i = 0; i < x; ++i)
        for (int j = 0; j < y; ++j)
            _map[i][j] = tile();
    _mapSize = std::make_pair(x, y);
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

Player *World::getPlayerByID(int playerID)
{
    for (const auto &column : _map)
        for (const auto &tile : column)
            for (const auto &player : tile._players)
                if (player->getPlayerID() == playerID)
                    return player.get();
    return nullptr;
}

Player *World::getPlayerByID(int playerID) const
{
    for (const auto &column : _map)
        for (const auto &tile : column)
            for (const auto &player : tile._players)
                if (player->getPlayerID() == playerID)
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
    Player *player = getPlayerByID(playerID);
    if (!player)
        return nullptr;
    return getTileAt(player->getPosition());
}

tile *World::getTileAt(position pos)
{
    if (pos.x >= _map.size() || pos.y >= _map[0].size())
        return nullptr;
    return &_map[pos.x][pos.y];
}

void World::setTileAt(position pos, ItemType itemType, int count)
{
    if (pos.x >= _map.size() || pos.y >= _map[0].size())
        return;

    std::vector<std::pair<ItemType, int>> &tileItems = _map[pos.x][pos.y]._items;
    auto it = std::find_if(tileItems.begin(), tileItems.end(), [itemType](const std::pair<ItemType, int> &item) { return item.first == itemType; });

    if (it != tileItems.end())
        it->second = count;
    else
        tileItems.emplace_back(itemType, count);
}

int World::getAvailableSlotsForTeam(const std::string &teamName) const
{
    int totalSlots = 0;
    bool teamFound = false;
    for (const auto &player : _players)
    {
        if (player->getTeam()._name == teamName)
        {
            teamFound = true;
            totalSlots += player->getTeam()._slotsAvailable;
        }
    }
    if (!teamFound)
        return -1;
    return totalSlots;
}
} // namespace zappy