#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Tna(std::vector<std::string> args, Client &client)
{
    (void)args;   // Unused parameter
    (void)client; // Unused parameter
    std::string response;
    std::vector<Team> teams = _world->getTeams();

    for (const auto &team : teams)
        response += "tna " + team._name + "\n";
    return response;
}
} // namespace zappy