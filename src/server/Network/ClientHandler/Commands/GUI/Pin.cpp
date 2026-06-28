#include "../Commands.hpp"

namespace zappy
{
std::string Commands::buildPinMessage(Player &player) const
{
    std::string message = "pin " + std::to_string(player.getId()) + " " + std::to_string(player.getPosition().x) + " " + std::to_string(player.getPosition().y) + " ";

    for (const int &count : player.getInventory().getItems())
        message += std::to_string(count) + " ";
    message += "\n";
    return message;
}

std::string Commands::Pin(std::vector<std::string> args, Client &)
{
    if (args.size() < 1)
        return "sbp\n";
    Player *player = _world->getPlayerById(std::stoi(args[0]));
    if (!player)
        return "ko\n";
    return buildPinMessage(*player);
}
} // namespace zappy
