#pragma once
#include <vector>
#include <memory>
#include "../../World/Items.hpp"
#include <unordered_map>

namespace zappy
{
    /// @brief What a single player is carrying: a count per ItemType.
    class Inventory
    {
        private:
            std::unordered_map<ItemType, int> _items; // Maybe change for futur bonus?
        public:
            /// @brief Builds an inventory with every item type present at count 0.
            Inventory();

            /// @brief Adds quantity of the given item.
            void addItem(ItemType type, int quantity = 1);

            /// @brief Removes quantity of the given item.
            void removeItem(ItemType type, int quantity = 1);

            /// @brief Formats the inventory as the protocol inventory reply string.
            std::string checkInventory() const;

            /// @brief Returns the held count for an item named by its protocol string.
            int getItemCount(const std::string &itemName) const;

            /// @brief Returns the held count for an item type.
            int getItemCount(const ItemType &itemType) const;

            /// @brief Mutable access to the underlying item-count map.
            std::unordered_map<ItemType, int>& getItems();
    };
} // namespace zappy
