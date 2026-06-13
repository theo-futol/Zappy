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
        public:
            World(int x, int y);

            void ressourcePassiveGeneration();
    };
} // namespace zappy
