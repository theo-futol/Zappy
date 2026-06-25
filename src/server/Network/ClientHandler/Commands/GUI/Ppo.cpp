#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Ppo(std::vector<std::string> args, Client &client)
{
    (void)client; // Unused parameter

    if (args.size() < 2)
        return "ko\n";
    Player *player = _world->getPlayerById(std::stoi(args[1]));
    if (!player)
        return "ko\n";
    position pos = player->getPosition();
    int orientation = player->getOrientation();
    return "ppo " + std::to_string(player->getId()) + " " + std::to_string(pos.x) + " " + std::to_string(pos.y) + " " + std::to_string(orientation) + "\n";
}
} // namespace zappy