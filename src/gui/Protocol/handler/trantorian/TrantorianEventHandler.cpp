#include "Protocol/handler/trantorian/TrantorianEventHandler.hpp"
#include "types/ResourceType.hpp"
#include <memory>

namespace Zappy
{

std::vector<std::string> TrantorianEventHandler::keys() const
{
    return {"pnw", "plv", "pin", "pex", "pfk", "pdr", "pgt", "pipi"};
}

Trantorian *TrantorianEventHandler::getTrantorian(GameState &state, int number)
{
    return dynamic_cast<Trantorian *>(state.getEntity({"player", number}));
}

void TrantorianEventHandler::transferResource(const std::vector<std::string> &args, GameState &state, int toPlayer)
{
    if (args.size() < 2)
        return;

    Trantorian *player = getTrantorian(state, entityNumber(args[0]));
    int index = toInt(args[1]);

    if (player == nullptr || index < 0 || index >= static_cast<int>(ResourceSet::Count))
        return;
    if (state.map().width() == 0 || state.map().height() == 0)
        return;

    ResourceType type = static_cast<ResourceType>(index);
    GridPosition position = player->position();
    ResourceSet &ground = state.map().at(position.x, position.y).resources();

    player->inventory().set(type, player->inventory().get(type) + toPlayer);
    ground.set(type, ground.get(type) - toPlayer);
}

void TrantorianEventHandler::handle(const std::string &key, const std::vector<std::string> &args, GameState &state)
{
    if (key == "pnw")
        handleTrantorianSpawned(args, state);
    else if (key == "plv")
        handleTrantorianLevelUpdated(args, state);
    else if (key == "pin")
        handleTrantorianInventoryUpdated(args, state);
    else if (key == "pex")
        handleTrantorianExpelled(args, state);
    else if (key == "pfk")
        handleTrantorianForked(args, state);
    else if (key == "pdr")
        handleTrantorianResourceDropped(args, state);
    else if (key == "pgt")
        handleTrantorianResourceTaken(args, state);
    else if (key == "pipi")
        handleTrantorianSnapshot(args, state);
}

void TrantorianEventHandler::handleTrantorianSnapshot(const std::vector<std::string> &args, GameState &state)
{
    if (args.size() < 12)
        return;

    Trantorian *player = getTrantorian(state, entityNumber(args[0]));

    if (player == nullptr)
        return;
    player->setPosition({toInt(args[1]), toInt(args[2])});
    player->setOrientation(toOrientation(toInt(args[3])));
    player->setLevel(toInt(args[4]));
    player->inventory() = toResources(args, 5);
}

void TrantorianEventHandler::handleTrantorianSpawned(const std::vector<std::string> &args, GameState &state)
{
    if (args.size() < 6)
        return;

    int player = entityNumber(args[0]);
    GridPosition position{toInt(args[1]), toInt(args[2])};
    Orientation orientation = toOrientation(toInt(args[3]));
    int level = toInt(args[4]);
    std::string team = args[5];

    state.addEntity(std::make_unique<Trantorian>(player, position, orientation, team, level, state.teamColor(team)));
}

void TrantorianEventHandler::handleTrantorianLevelUpdated(const std::vector<std::string> &args, GameState &state)
{
    if (args.size() < 2)
        return;
    Trantorian *player = getTrantorian(state, entityNumber(args[0]));
    int level = toInt(args[1]);

    if (player == nullptr)
        return;
    player->setLevel(level);
}

void TrantorianEventHandler::handleTrantorianInventoryUpdated(const std::vector<std::string> &args, GameState &state)
{
    if (args.size() < 10)
        return;

    Trantorian *player = getTrantorian(state, entityNumber(args[0]));
    if (player == nullptr)
        return;
    player->inventory() = toResources(args, 3);
}

void TrantorianEventHandler::handleTrantorianExpelled(const std::vector<std::string> &args, GameState &state)
{
    (void)args;
    (void)state;
}

void TrantorianEventHandler::handleTrantorianForked(const std::vector<std::string> &args, GameState &state)
{
    (void)args;
    (void)state;
}

void TrantorianEventHandler::handleTrantorianResourceDropped(const std::vector<std::string> &args, GameState &state)
{
    transferResource(args, state, -1);
}

void TrantorianEventHandler::handleTrantorianResourceTaken(const std::vector<std::string> &args, GameState &state)
{
    transferResource(args, state, +1);
}

} // namespace Zappy
