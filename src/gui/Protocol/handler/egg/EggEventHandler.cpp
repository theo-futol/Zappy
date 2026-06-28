#include "Protocol/handler/egg/EggEventHandler.hpp"
#include "Model/egg/Egg.hpp"
#include "Model/trantorian/Trantorian.hpp"
#include <memory>

namespace Zappy
{

std::vector<std::string> EggEventHandler::keys() const
{
    return {"enw", "ebo"};
}

std::string EggEventHandler::parentTeam(GameState &state, int parentNumber)
{
    IEntity *parent = state.getEntity({"player", parentNumber});
    Trantorian *trantorian = dynamic_cast<Trantorian *>(parent);

    if (trantorian == nullptr)
        return "";
    return trantorian->team();
}

void EggEventHandler::handleEggLaid(const std::vector<std::string> &args, GameState &state)
{
    if (args.size() < 4)
        return;

    int number = entityNumber(args[0]);
    GridPosition position{toInt(args[2]), toInt(args[3])};
    std::string team = parentTeam(state, entityNumber(args[1]));

    state.addEntity(std::make_unique<Egg>(number, position, team, state.teamColor(team)));
}

void EggEventHandler::handleEggHatch(const std::vector<std::string> &args, GameState &state)
{
    if (args.empty())
        return;
    state.removeEntity({"egg", entityNumber(args[0])});
}

void EggEventHandler::handle(const std::string &key, const std::vector<std::string> &args, GameState &state)
{
    if (key == "enw")
        handleEggLaid(args, state);
    else if (key == "ebo")
        handleEggHatch(args, state);
}

} // namespace Zappy
