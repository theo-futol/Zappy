#pragma once
#include <vector>
#include <memory>
#include "../../World/Items.hpp"
#include <unordered_map>

namespace zappy
{
    class Inventory
    {
        private:
            std::unordered_map<ItemType, int> _items; // Maybe change for futur bonus?
        public:
            Inventory();

            void addItem(ItemType type, int quantity = 1);
            void removeItem(ItemType type, int quantity = 1);
            std::string checkInventory() const;
    };
} // namespace zappy
