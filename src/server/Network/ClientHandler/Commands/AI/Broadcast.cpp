#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Broadcast(std::vector<std::string> args, Client &client)
{
    Player *player = _world->getPlayerById(client.getPlayerId());
    if (!player || args.size() != 1 || args[0].empty())
    {
        Logger::log("8421", "Broadcast : failed", {{"player_id", std::to_string(client.getPlayerId())}});
        return "ko\n";
    }
    for (const auto &target : _world->getPlayers())
        if (target->getId() != player->getId())
            player->addMessageToQueue("message " + std::to_string(Direction::getDirectionValue(player->getDirectionTo(*target, _world->getMapSize()))) + ", " + args[0] + "\n",
                                      player->getDistanceTo(*target, _world->getMapSize()) * BROADCAST_MESSAGE_TIME_PER_TILE, player->getFd());
    player->sortQueueByTimeNeeded();
    _broadcastQueue->push("pbc " + std::to_string(player->getId()) + " " + args[0] + "\n");
    Logger::log("011", "Broadcast : message sent", {{"player_id", std::to_string(player->getId())}, {"message", args[0]}});
    return "ok\n";
}
} // namespace zappy
