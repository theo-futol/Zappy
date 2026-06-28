#include "Protocol/handler/team/TeamEventHandler.hpp"

namespace Zappy
{

std::vector<std::string> TeamEventHandler::keys() const
{
    return {"tna"};
}

void TeamEventHandler::handle(const std::string &key, const std::vector<std::string> &args, GameState &state)
{
    (void)key;
    if (!args.empty())
        state.addTeam(args[0]);
}

} // namespace Zappy
