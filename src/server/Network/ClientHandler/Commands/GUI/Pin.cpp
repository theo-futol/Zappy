#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Pin(std::vector<std::string> args, Client &)
{
    if (args.size() < 2)
        return "ko\n";
    Player *player = _world->getPlayerById(std::stoi(args[1]));
    if (!player)
        return "ko\n";
    std::string response = "pin " + std::to_string(player->getId()) + " " + std::to_string(player->getPosition().x) + " " + std::to_string(player->getPosition().y) + " ";

    for (const int &count : player->getInventory().getItems())
        response += std::to_string(count) + " ";
    response += "\n";
    return response;
}
} // namespace zappy
