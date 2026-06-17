#include "Commands.hpp"

namespace zappy
{
std::string Commands::Broadcast(std::vector<std::string> args, Client &client)
{
    Player *player = _world->getPlayerByFd(client.getFd());
    if (!player)
        return "ko\n";
    position playerPos = player->getPosition();
    for (std::shared_ptr<Player> player : _world->getPlayers()) // TO DO : Use the distance to determine the time needed to receive the message ?
        if (player->getFd() != client.getFd())
            player->writeToClient("message " + std::to_string(player->getDirectionTo(playerPos, _world->getMapSize()) / 45 + 1) + ", " + args[0] + "\n");
    return "ok\n";
}
} // namespace zappy
