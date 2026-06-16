#include "Commands.hpp"

namespace zappy
{
std::string Commands::getInventory(std::vector<std::string> args, Client &client)
{
    (void)args;
    Player *player = _world->getPlayerByFd(client.getFd());
    if (player == nullptr)
        return "ko\n";
    return player->getInventory().checkInventory();
}
} // namespace zappy
