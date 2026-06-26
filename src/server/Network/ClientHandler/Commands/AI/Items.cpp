#include "../../../../Simulation/Player/Inventory/Inventory.hpp"
#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Take(std::vector<std::string> args, Client &client, std::vector<std::unique_ptr<Client>> &clients)
{
    (void)clients;
    tile *currentTile = _world->getTileAt(client.getPlayerId());
    if (args.size() < 1 || !currentTile)
        return "ko\n";
    ItemType itemType = stringToItemType(args[0]);
    if (itemType == ItemType::UNKNOWN)
        return "ko\n";
    Player *player = _world->getPlayerById(client.getPlayerId());
    position pos = player->getPosition();
    for (auto it = currentTile->_items.begin(); it != currentTile->_items.end(); it++)
    {
        if (itemType == it->first)
        {
            if (it->second <= 0)
            {
                Logger::log("8426", "Take : failed, item not on tile",
                            {{"player_id", std::to_string(player->getId())}, {"item", args[0]}, {"x", std::to_string(pos.x)}, {"y", std::to_string(pos.y)}});
                return "ko\n";
            }
            it->second--;
            player->getInventory().addItem(itemType);
            break;
        }
    }
    _broadcastQueue->push("pgt " + std::to_string(player->getId()) + " " + std::to_string(static_cast<int>(stringToItemType(args[0]))) + "\n");
    Logger::log("01A", "Take : item taken", {{"player_id", std::to_string(player->getId())}, {"item", args[0]}, {"x", std::to_string(pos.x)}, {"y", std::to_string(pos.y)}});
    return "ok\n";
}

std::string Commands::Set(std::vector<std::string> args, Client &client, std::vector<std::unique_ptr<Client>> &clients)
{
    (void)clients;
    tile *currentTile = _world->getTileAt(client.getPlayerId());
    if (args.size() < 1 || !currentTile)
        return "ko\n";
    ItemType itemType = stringToItemType(args[0]);
    if (itemType == ItemType::UNKNOWN)
        return "ko\n";
    Player *player = _world->getPlayerById(client.getPlayerId());
    position pos = player->getPosition();
    int itemCount = player->getInventory().getItemCount(itemType);
    if (itemCount <= 0)
    {
        Logger::log("8427", "Set : failed", {{"player_id", std::to_string(player->getId())}, {"item", args[0]}});
        return "ko\n";
    }
    player->getInventory().removeItem(itemType);
    for (auto it = currentTile->_items.begin(); it != currentTile->_items.end(); it++)
        if (itemType == it->first)
        {
            it->second++;
            break;
        }
    _broadcastQueue->push("pdr " + std::to_string(player->getId()) + " " + std::to_string(static_cast<int>(stringToItemType(args[0]))) + "\n");
    Logger::log("01B", "Set : item dropped", {{"player_id", std::to_string(player->getId())}, {"item", args[0]}, {"x", std::to_string(pos.x)}, {"y", std::to_string(pos.y)}});
    return "ok\n";
}
} // namespace zappy
