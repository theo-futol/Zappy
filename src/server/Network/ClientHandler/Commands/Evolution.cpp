#include "Commands.hpp"

namespace zappy
{
std::string Commands::Fork(std::vector<std::string> args, Client &client)
{
    (void)args; // Unused parameter
    position playerPos = _world->getPlayerByFd(client.getFd())->getPosition();

    _world->setTileAt(_world->getPlayerByFd(client.getFd())->getPosition(), ItemType::EGG, 1);
    _world->getPlayerByFd(client.getFd())->getTeam().addEgg(playerPos);
    return "ok\n";
}

std::string Commands::Incantation(std::vector<std::string> args, Client &client)
{
    (void)args;                              // Unused parameter
    (void)client;                            // Unused parameter
    return "Incantation command executed\n"; // Placeholder return value
}
} // namespace zappy
