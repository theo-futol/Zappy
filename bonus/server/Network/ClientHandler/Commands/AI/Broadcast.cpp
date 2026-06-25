#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Broadcast(std::vector<std::string> args, Client &client)
{
    Player *player = _world->getPlayerByFd(client.getFd());
    if (!player || args.size() != 1 || args[0].empty())
        return "ko\n";
    for (const auto &target : _world->getPlayers())
        if (target->getFd() != client.getFd())
            player->addMessageToQueue("message " + std::to_string(player->getDirectionTo(*target, _world->getMapSize()) / 45 + 1) + ", " + args[0] + "\n",
                                      player->getDistanceTo(*target, _world->getMapSize()) * BROADCAST_MESSAGE_TIME_PER_TILE, player->getFd());
    player->sortQueueByTimeNeeded();
    _broadcastQueue->push("pbc " + std::to_string(player->getFd()) + " " + args[0] + "\n");
    std::cout << "Player " << player->getFd() << " is broadcasting\n";
    return "ok\n";
}
} // namespace zappy
