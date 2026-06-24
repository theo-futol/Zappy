#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Connect_nbr(std::vector<std::string> args, Client &client)
{
    (void)args; // Unused parameter
    Player *player = _world->getPlayerByFd(client.getFd());
    if (!player)
        return "ko\n";
    return std::to_string(player->getTeam().getAvailableSlots()) + "\n";
}
} // namespace zappy
