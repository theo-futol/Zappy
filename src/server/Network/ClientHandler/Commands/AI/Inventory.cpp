#include "../Commands.hpp"

namespace zappy
{
std::string Commands::getInventory(std::vector<std::string> args, Client &client)
{
    (void)args;
    Player *player = _world->getPlayerById(client.getPlayerId());
    if (player == nullptr)
        return "ko\n";
    Inventory &inv = player->getInventory();
    Logger::log("018", "Inventory : response sent",
                {{"player_id", std::to_string(player->getId())},
                 {"food", std::to_string(inv.getItemCount(ItemType::FOOD))},
                 {"linemate", std::to_string(inv.getItemCount(ItemType::LINEMATE))},
                 {"deraumere", std::to_string(inv.getItemCount(ItemType::DERAUMERE))},
                 {"sibur", std::to_string(inv.getItemCount(ItemType::SIBUR))},
                 {"mendiane", std::to_string(inv.getItemCount(ItemType::MENDIANE))},
                 {"phiras", std::to_string(inv.getItemCount(ItemType::PHIRAS))},
                 {"thystame", std::to_string(inv.getItemCount(ItemType::THYSTAME))}});
    return inv.checkInventory();
}
} // namespace zappy
