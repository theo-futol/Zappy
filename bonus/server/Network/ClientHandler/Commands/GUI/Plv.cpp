#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Plv(std::vector<std::string> args, Client &)
{
    Player *player = _world->getPlayerById(parsePlayerIdArg(args));
    if (!player)
        return "ko\n";
    return "plv " + std::to_string(player->getId()) + " " + std::to_string(player->getLevel()) + "\n";
}
} // namespace zappy