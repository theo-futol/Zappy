#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Eject(std::vector<std::string> args, Client &client)
{
    (void)args; // Unused parameter
    Player *player = _world->getPlayerByFd(client.getFd());
    if (!player)
        return "ko\n"; // Player not found
    position playerPos = player->getPosition();
    position nextPos = player->nextPosition(_world->getMapSize());
    bool hasEjectedEggs = player->getTeam().hasEggAtPosition(playerPos);
    bool hasEjectedPlayers = false;
    std::vector<Player *> ejectedPlayers;

    // move all players to nextPosition
    tile *currentTile = _world->getTileAt(playerPos);
    _world->sendMessageToPlayersThatAreOnTile(playerPos, "eject: " + std::to_string(player->getRotation()) + "\n");
    for (const auto &otherPlayer : currentTile->_players)
        if (otherPlayer->getFd() != player->getFd())
            ejectedPlayers.push_back(otherPlayer);
    for (const auto &otherPlayer : ejectedPlayers)
    {
        hasEjectedPlayers = true;
        otherPlayer->setPosition(nextPos.x, nextPos.y, _world->getMapSize());
        _world->getTileAt(nextPos)->_players.push_back(otherPlayer);
    }
    currentTile->_players.erase(std::remove_if(currentTile->_players.begin(), currentTile->_players.end(), [&player](const Player *p) { return p->getFd() != player->getFd(); }),
                                currentTile->_players.end());
    // destroy all eggs on the tile
    _world->setTileAt(playerPos, ItemType::EGG, 0);
    player->getTeam().removeEgg(playerPos, -1, -1);
    _broadcastQueue->push("pex " + std::to_string(player->getFd()) + "\n");
    return hasEjectedEggs || hasEjectedPlayers ? "ok\n" : "ko\n";
}
} // namespace zappy
