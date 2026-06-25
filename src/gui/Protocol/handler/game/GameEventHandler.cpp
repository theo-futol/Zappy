#include "Protocol/handler/game/GameEventHandler.hpp"

#include <cstddef>

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

void GameEventHandler::handleBroadcast(const std::vector<std::string> &args, GameState &state)
{
    if (args.empty())
        return;

    std::string text = "#" + args[0] + ":";

    for (std::size_t i = 1; i < args.size(); ++i)
        text += " " + args[i];
    state.addMessage(text, BroadcastColor);
}

void GameEventHandler::handleServerMessage(const std::vector<std::string> &args, GameState &state)
{
    std::string text;

    for (std::size_t i = 0; i < args.size(); ++i)
        text += (i == 0 ? "" : " ") + args[i];
    if (!text.empty())
        state.addMessage(text, ServerColor);
}

void GameEventHandler::handle(const std::string &key, const std::vector<std::string> &args, GameState &state)
{
    if (key == "sgt" || key == "sst")
        handleTimeUnit(args, state);
    else if (key == "seg")
        handleEndGame(args, state);
    else if (key == "pbc")
        handleBroadcast(args, state);
    else if (key == "smg")
        handleServerMessage(args, state);
    // suc / sbp carry no model state (command replies) -> ignored.
}

} // namespace Zappy
