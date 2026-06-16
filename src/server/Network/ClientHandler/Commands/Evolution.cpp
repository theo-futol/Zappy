#include "Commands.hpp"

namespace zappy
{
std::string Commands::Fork(std::vector<std::string> args, Client &client)
{
    (void)args;                       // Unused parameter
    position playerPos = _world->getPlayerByID(client.getPlayerID())->getPosition();

    _world->setTileAt(_world->getPlayerByID(client.getPlayerID())->getPosition(), ItemType::EGG, 1);
    _world->getPlayerByID(client.getPlayerID())->getTeam().addEgg(playerPos);
    return "ok\n";
}

std::string Commands::Incantation(std::vector<std::string> args, Client &client)
{
    (void)args;                              // Unused parameter
    (void)client;                            // Unused parameter
    return "Incantation command executed\n"; // Placeholder return value
}
} // namespace zappy
