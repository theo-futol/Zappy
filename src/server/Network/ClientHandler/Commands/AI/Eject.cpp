#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Eject(std::vector<std::string> args, Client &client, std::vector<std::unique_ptr<Client>> &clients)
{
    (void)args; // Unused parameter
    (void)clients;
    Player *player = _world->getPlayerById(client.getPlayerId());
    if (!player)
        return "ko\n"; // Player not found
    position playerPos = player->getPosition();
    position nextPos = player->nextPosition(_world->getMapSize());
    bool hasEjectedEggs = player->getTeam().hasEggAtPosition(playerPos);
    bool hasEjectedPlayers = false;

    // move all players to nextPosition
    tile *currentTile = _world->getTileAt(playerPos);
    _world->sendMessageToPlayersThatAreOnTile(playerPos, "eject: " + std::to_string(player->getRotation()) + "\n", clients);
    for (const auto &otherPlayer : currentTile->_players)
    {
        if (otherPlayer->getId() != player->getId() && nextPos != playerPos)
        {
            hasEjectedPlayers = true;
            otherPlayer->setPosition(nextPos.x, nextPos.y, _world->getMapSize());
            _world->getTileAt(nextPos)->_players.push_back(otherPlayer);
        }
    }
    currentTile->_players.erase(std::remove_if(currentTile->_players.begin(), currentTile->_players.end(),
                                               [&player, &playerPos](const Player *p) { return p->getId() != player->getId() && p->getPosition() != playerPos; }),
                                currentTile->_players.end());
    // destroy all eggs on the tile
    Logger::log("8443", "Player : egg destroyed by ejection",
                {{"player_id", std::to_string(player->getId())}, {"x", std::to_string(playerPos.x)}, {"y", std::to_string(playerPos.y)}});
    _world->setTileAt(playerPos, ItemType::EGG, 0);
    player->getTeam().removeEgg(playerPos, -1, -1);
    _broadcastQueue->push("pex " + std::to_string(player->getId()) + "\n");
    if (hasEjectedEggs || hasEjectedPlayers)
    {
        Logger::log("019", "Eject : players ejected", {{"player_id", std::to_string(player->getId())}, {"x", std::to_string(nextPos.x)}, {"y", std::to_string(nextPos.y)}});
        return "ok\n";
    }
    Logger::log("8425", "Eject : failed, no players to eject", {{"player_id", std::to_string(player->getId())}});
    return "ko\n";
}
} // namespace zappy
