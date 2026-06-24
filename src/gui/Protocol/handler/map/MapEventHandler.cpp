#include "Protocol/handler/map/MapEventHandler.hpp"
#include "Model/gamestate/GameState.hpp"
#include <vector>

namespace Zappy
{

std::vector<std::string> MapEventHandler::keys() const
{
    return {"msz", "bct", "mct"};
}

void MapEventHandler::handleMapSize(const std::vector<std::string> &args, GameState &state)
{
    if (args.size() < 2)
        return;
    state.map().resize(toInt(args[0]), toInt(args[1]));
}

void MapEventHandler::handleTileContent(const std::vector<std::string> &args, GameState &state)
{
    if (args.size() < 9)
        return;
    state.map().at(toInt(args[0]), toInt(args[1])).resources() = toResources(args, 2);
}

void MapEventHandler::handle(const std::string &key, const std::vector<std::string> &args, GameState &state)
{
    if (key == "msz")
        handleMapSize(args, state);
    else if (key == "bct")
        handleTileContent(args, state);
}

} // namespace Zappy
