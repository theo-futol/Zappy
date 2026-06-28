#include "../Commands.hpp"

namespace zappy
{
std::string Commands::buildPipiMessage(Player &player) const
{
    std::string message = "pipi " + std::to_string(player.getId()) + " " + std::to_string(player.getPosition().x) + " " + std::to_string(player.getPosition().y) + " " +
                          std::to_string(player.getOrientation()) + " " + std::to_string(player.getLevel()) + " ";
    for (const int &count : player.getInventory().getItems())
        message += std::to_string(count) + " ";
    message += "\n";
    return message;
}
} // namespace zappy
