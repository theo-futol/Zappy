#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Connect_nbr(std::vector<std::string> args, Client &client)
{
    (void)args; // Unused parameter
    return std::to_string(_world->getPlayerByFd(client.getFd())->getTeam().getAvailableSlots()) + "\n";
}
} // namespace zappy
