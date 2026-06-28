#include "../Commands.hpp"

namespace zappy
{
std::string Commands::buildPpoMessage(Player &player) const
{
    position pos = player.getPosition();
    return "ppo " + std::to_string(player.getId()) + " " + std::to_string(pos.x) + " " + std::to_string(pos.y) + " " + std::to_string(player.getOrientation()) + "\n";
}

std::string Commands::Ppo(std::vector<std::string> args, Client &)
{

    if (args.size() < 2)
        return "ko\n";
    Player *player = _world->getPlayerById(std::stoi(args[1]));
    if (!player)
        return "ko\n";
    return buildPpoMessage(*player);
}
} // namespace zappy
