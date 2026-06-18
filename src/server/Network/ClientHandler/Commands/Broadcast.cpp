#include "Commands.hpp"

namespace zappy
{
std::string Commands::Broadcast(std::vector<std::string> args, Client &client)
{
    Player *player = _world->getPlayerByFd(client.getFd());
    if (!player)
        return "ko\n";
    for (std::shared_ptr<Player> target : _world->getPlayers()) // TO DO : Use the distance to determine the time needed to receive the message ?
        if (target->getFd() != client.getFd())
            target->writeToClient("message " + std::to_string(player->getDirectionTo(*target, _world->getMapSize()) / 45 + 1) + ", " + args[0] +
                                  "\n"); // TO DO : ENSURE THE DIRECTION IS CORRECT
    _broadcastQueue->push("pbc " + std::to_string(player->getFd()) + " " + args[0] + "\n");
    return "ok\n";
}
} // namespace zappy

// To propagate the message to each player:
// - We will need the distance from the player to the sender
// Using that distance, we can determine the time needed for the message to be received by the player
// The next step is to use thread to send the message to the player after the time needed has passed
// One thread will be created to send the message to all players when the time needed has passed