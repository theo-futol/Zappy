#include "Commands.hpp"

namespace zappy
{
std::string Commands::Connect_nbr(std::vector<std::string> args, Client &client)
{
    (void)args;                              // Unused parameter
    return std::to_string(_world->getPlayerByID(client.getPlayerID())->getTeam().getAvailableSlots()) + "\n";
}
} // namespace zappy
