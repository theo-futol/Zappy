#pragma once
#include <vector>
#include <memory>
#include "../../World/Items.hpp"

namespace zappy
{
    /// @brief What a single player is carrying: a count per ItemType.
    ///        Stored as a vector indexed by ItemType so iteration order
    ///        matches the protocol's fixed food/linemate/.../thystame order.
    class Inventory
    {
        private:
            std::vector<int> _items;
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

            /// @brief Mutable access to the underlying item counts, indexed by ItemType.
            std::vector<int>& getItems();
    };
} // namespace zappy
