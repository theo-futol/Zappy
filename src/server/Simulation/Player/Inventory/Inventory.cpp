#include "Inventory.hpp"

namespace zappy
{
Inventory::Inventory()
{
    for (int i = static_cast<int>(ItemType::FOOD); i <= static_cast<int>(ItemType::THYSTAME); ++i)
        _items[static_cast<ItemType>(i)] = 0;
}

void Inventory::addItem(ItemType type, int quantity)
{
    _items[type] += quantity;
}

void Inventory::removeItem(ItemType type, int quantity)
{
    auto it = _items.find(type);
    if (it != _items.end())
    {
        it->second -= quantity;
        if (it->second < 0)
            it->second = 0;
    }
}

std::string Inventory::checkInventory() const
{
    std::string result = "[";
    for (auto it = _items.begin(); it != _items.end(); ++it)
    {
        result += itemTypeToString(it->first) + " " + std::to_string(it->second);
        if (std::next(it) != _items.end())
            result += ", ";
    }
    result += "]\n";
    return result;
}

int Inventory::getItemCount(const std::string &itemName) const
{
    ItemType type = stringToItemType(itemName);
    return getItemCount(type);
}

int Inventory::getItemCount(const ItemType &itemType) const
{
    auto it = _items.find(itemType);
    if (it != _items.end())
        return it->second;
    return 0;
}

std::unordered_map<ItemType, int> &Inventory::getItems()
{
    return _items;
}

} // namespace zappy