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
    {
        if (target->getId() == player->getId())
            continue;

        int direction = Direction::getDirectionValue(target->getDirectionTo(*player, _world->getMapSize()));
        int delay = player->getDistanceTo(*target, _world->getMapSize()) * BROADCAST_MESSAGE_TIME_PER_TILE;
        target->addMessageToQueue("message " + std::to_string(direction) + ", " + args[0] + "\n", delay, target->getFd());
        target->sortQueueByTimeNeeded();
    }
    _broadcastQueue->push("pbc " + std::to_string(player->getId()) + " " + args[0] + "\n");
    Logger::log("011", "Broadcast : message sent", {{"player_id", std::to_string(player->getId())}, {"message", args[0]}});
    return "ok\n";
}
} // namespace zappy
