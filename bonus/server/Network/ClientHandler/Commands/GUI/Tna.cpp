#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Tna(std::vector<std::string>, Client &)
{
    std::string response;
    std::vector<std::shared_ptr<Team>> &teams = _world->getTeams();

    for (const auto &team : teams)
        response += "tna " + team->_name + "\n";
    return response;
}
} // namespace zappy