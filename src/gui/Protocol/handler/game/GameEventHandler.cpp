#include "Protocol/handler/game/GameEventHandler.hpp"

namespace Zappy
{

std::vector<std::string> GameEventHandler::keys() const
{
    return {"sgt", "sst", "seg", "smg", "suc", "sbp", "pbc"};
}

void GameEventHandler::handleTimeUnit(const std::vector<std::string> &args, GameState &state)
{
    if (args.empty())
        return;
    state.setTimeUnit(toInt(args[0]));
}

void GameEventHandler::handleEndGame(const std::vector<std::string> &args, GameState &state)
{
    if (args.empty())
        return;
    state.setWinner(args[0]);
}

void GameEventHandler::handle(const std::string &key, const std::vector<std::string> &args, GameState &state)
{
    if (key == "sgt" || key == "sst")
        handleTimeUnit(args, state);
    else if (key == "seg")
        handleEndGame(args, state);
    // smg / suc / sbp / pbc carry no model state (server logs, broadcast) -> shown at render time.
}

} // namespace Zappy
