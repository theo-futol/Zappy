#include "Inventory.hpp"

namespace zappy
{
Inventory::Inventory() : _items(static_cast<size_t>(ItemType::THYSTAME) + 1, 0)
{
}

void Inventory::addItem(ItemType type, int quantity)
{
    if (static_cast<size_t>(type) >= _items.size())
        return;
    _items[static_cast<size_t>(type)] += quantity;
}

void Inventory::removeItem(ItemType type, int quantity)
{
    if (static_cast<size_t>(type) >= _items.size())
        return;
    int &count = _items[static_cast<size_t>(type)];
    count -= quantity;
    if (count < 0)
        count = 0;
}

std::string Inventory::checkInventory() const
{
    std::string result = "[";
    for (size_t i = 0; i < _items.size(); ++i)
    {
        result += itemTypeToString(static_cast<ItemType>(i)) + " " + std::to_string(_items[i]);
        if (i + 1 != _items.size())
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
    if (static_cast<size_t>(itemType) >= _items.size())
        return 0;
    return _items[static_cast<size_t>(itemType)];
}

std::vector<int> &Inventory::getItems()
{
    return _items;
}

} // namespace zappy