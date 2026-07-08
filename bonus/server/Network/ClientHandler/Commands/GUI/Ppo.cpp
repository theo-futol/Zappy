#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Ppo(std::vector<std::string> args, Client &)
{
    Player *player = _world->getPlayerById(parsePlayerIdArg(args));
    if (!player)
        return "ko\n";
    position pos = player->getPosition();
    int orientation = player->getOrientation();
    return "ppo " + std::to_string(player->getId()) + " " + std::to_string(pos.x) + " " + std::to_string(pos.y) + " " + std::to_string(orientation) + "\n";
}
} // namespace zappy