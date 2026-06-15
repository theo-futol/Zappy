#include "../../../Simulation/Player/Inventory/Inventory.hpp"
#include "Commands.hpp"

namespace zappy
{
std::string Commands::Take(std::vector<std::string> args, Client &client)
{
    int playerID = client.getPlayerID();
    tile *currentTile = _world->getTileAt(playerID);
    if (args.size() < 2 || !currentTile)
        return "ko\n";
    ItemType itemType = stringToItemType(args[1]);
    if (itemType == ItemType::UNKNOWN)
        return "ko\n";
    for (auto it = currentTile->_items.begin(); it != currentTile->_items.end(); it++)
    {
        if (itemType == it->first)
        {
            if (it->second <= 0)
                return "ko\n";
            it->second--;
            _world->getPlayerByID(playerID)->getInventory().addItem(itemType);
            break;
        }
    }
    return "ok\n";
}

std::string Commands::Set(std::vector<std::string> args, Client &client)
{
    int playerID = client.getPlayerID();
    tile *currentTile = _world->getTileAt(playerID);
    if (args.size() < 2 || !currentTile)
        return "ko\n";
    ItemType itemType = stringToItemType(args[1]);
    if (itemType == ItemType::UNKNOWN)
        return "ko\n";
    int itemCount = _world->getPlayerByID(playerID)->getInventory().getItemCount(itemType);
    if (itemCount <= 0)
        return "ko\n";
    _world->getPlayerByID(playerID)->getInventory().removeItem(itemType);
    for (auto it = currentTile->_items.begin(); it != currentTile->_items.end(); it++)
        if (itemType == it->first)
        {
            it->second++;
            break;
        }
    return "ok\n";
}
} // namespace zappy
