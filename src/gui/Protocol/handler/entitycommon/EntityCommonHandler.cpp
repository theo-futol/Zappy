#include "Protocol/handler/entitycommon/EntityCommonHandler.hpp"

namespace Zappy
{

std::vector<std::string> EntityCommonHandler::keys() const
{
    return {"ppo", "pdi", "edi"};
}

void EntityCommonHandler::handlePosition(const std::vector<std::string> &args, GameState &state)
{
    if (args.size() < 4)
        return;

    IEntity *entity = state.getEntity({"player", entityNumber(args[0])});

    if (entity == nullptr)
        return;
    entity->setPosition({toInt(args[1]), toInt(args[2])});
    entity->setOrientation(toOrientation(toInt(args[3])));
}

void EntityCommonHandler::handleRemoval(const std::vector<std::string> &args, GameState &state, const std::string &entityType)
{
    if (args.empty())
        return;
    state.removeEntity({entityType, entityNumber(args[0])});
}

void EntityCommonHandler::handle(const std::string &key, const std::vector<std::string> &args, GameState &state)
{
    if (key == "ppo")
        handlePosition(args, state);
    else if (key == "pdi")
        handleRemoval(args, state, "player");
    else if (key == "edi")
        handleRemoval(args, state, "egg");
}

} // namespace Zappy
