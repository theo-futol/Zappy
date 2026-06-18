#include "Commands.hpp"

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
    bool hasEjectedPlayers = player->getTeam().hasEggAtPosition(playerPos);

    // move all players to nextPosition
    tile *currentTile = _world->getTileAt(playerPos);
    for (const auto &otherPlayer : currentTile->_players)
        if (otherPlayer->getFd() != player->getFd())
        {
            otherPlayer->setPosition(nextPos.x, nextPos.y, _world->getMapSize());
            _world->getTileAt(nextPos)->_players.push_back(otherPlayer);
            _world->getTileAt(playerPos)->_players.erase(std::remove(_world->getTileAt(playerPos)->_players.begin(), _world->getTileAt(playerPos)->_players.end(), otherPlayer),
                                                         _world->getTileAt(playerPos)->_players.end());
        }
    // destroy all eggs on the tile
    _world->setTileAt(playerPos, ItemType::EGG, 0);
    player->getTeam().removeEgg(playerPos, -1);
    _world->sendMessageToPlayersThatAreOnTile(playerPos, "eject: " + std::to_string(player->getRotation()) + "\n");
    return hasEjectedPlayers || !(currentTile->_players.empty()) ? "ok\n" : "ko\n";
}
} // namespace zappy
