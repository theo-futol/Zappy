#pragma once
#include <vector>
#include <memory>
#include <algorithm>
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
    
    using Map = std::vector<std::vector<tile>>;

    class World
    {
        private:
            std::vector<std::shared_ptr<Player>> _players;
            Map _map;
            std::pair<int, int> _mapSize;
        public:
            World(int x, int y);
            ~World() = default;

            void ressourcePassiveGeneration();
            Player *getPlayerByID(int playerID);
            Player *getPlayerByID(int playerID) const;
            std::pair<int, int> getMapSize() const;

            tile *getTileAt(int playerID);
            tile *getTileAt(position pos);

            int getAvailableSlotsForTeam(const std::string &teamName) const;
        };
} // namespace zappy
