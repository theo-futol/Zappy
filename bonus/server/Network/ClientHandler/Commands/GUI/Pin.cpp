#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Pin(std::vector<std::string> args, Client &)
{
    Player *player = _world->getPlayerById(parsePlayerIdArg(args));
    if (!player)
        return "ko\n";
    std::string response = "pin " + std::to_string(player->getId()) + " " + std::to_string(player->getPosition().x) + " " + std::to_string(player->getPosition().y) + " ";

    for (const int &count : player->getInventory().getItems())
        response += std::to_string(count) + " ";
    response += "\n";
    return response;
}
} // namespace zappy
