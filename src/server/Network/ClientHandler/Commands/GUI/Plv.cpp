#include "../Commands.hpp"

namespace zappy
{
std::string Commands::buildPlvMessage(Player &player) const
{
    return "plv " + std::to_string(player.getId()) + " " + std::to_string(player.getLevel()) + "\n";
}

std::string Commands::Plv(std::vector<std::string> args, Client &)
{
    if (args.size() < 1)
        return "sbp\n";
    try
    {
        Player *player = _world->getPlayerById(std::stoi(args[0]));
        if (!player)
            return "ko\n";
        return buildPlvMessage(*player);
    }
    catch (...)
    {
        return "sbp\n";
    }
}
} // namespace zappy
