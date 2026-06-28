#include "Protocol/handler/entitycommon/EntityCommonHandler.hpp"

#include <memory>

#include "Model/trantorian/Trantorian.hpp"
#include "types/Color.hpp"

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

    int number = entityNumber(args[0]);
    GridPosition position{toInt(args[1]), toInt(args[2])};
    Orientation orientation = toOrientation(toInt(args[3]));
    IEntity *entity = state.getEntity({"player", number});

    // The server does not always announce pre-existing players (no pnw dump on connect),
    // yet it streams their ppo. Create the player on first sight so it is locatable
    // (team/level fill in later via pnw/plv); otherwise broadcasts and rendering for it
    // would silently vanish.
    if (entity == nullptr)
    {
        state.addEntity(std::make_unique<Trantorian>(number, position, orientation, std::string(), 1, Color{0.80f, 0.80f, 0.85f, 1.0f}));
        return;
    }
    entity->setPosition(position);
    entity->setOrientation(orientation);
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
