#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Forward(std::vector<std::string>, Client &client, std::vector<std::unique_ptr<Client>> &)
{
    Player *player = _world->getPlayerById(client.getPlayerId());
    if (!player)
    {
        Logger::log("8424", "Forward : failed", {{"player_id", std::to_string(client.getPlayerId())}});
        return "dead\n";
    }

    position oldPos = player->getPosition();
    player->move(_world->getMapSize());
    _world->removePlayerFromTile(player, oldPos);
    _world->addPlayerToTile(player, player->getPosition());
    position newPos = player->getPosition();
    _broadcastQueue->push(buildPipiMessage(*player));
    Logger::log("014", "Forward : player moved", {{"player_id", std::to_string(player->getId())}, {"x", std::to_string(newPos.x)}, {"y", std::to_string(newPos.y)}});
    return "ok\n";
}
std::string Commands::Right(std::vector<std::string>, Client &client, std::vector<std::unique_ptr<Client>> &)
{
    Player *player = _world->getPlayerById(client.getPlayerId());
    if (!player)
        return "dead\n";
    int newRotation = (player->getRotation() + 90) % 360;

    player->setRotation(newRotation);
    _broadcastQueue->push(buildPipiMessage(*player));
    Logger::log("015", "Right : player turned right", {{"player_id", std::to_string(player->getId())}, {"rotation", std::to_string(player->getRotation())}});
    return "ok\n";
}
std::string Commands::Left(std::vector<std::string>, Client &client, std::vector<std::unique_ptr<Client>> &)
{
    Player *player = _world->getPlayerById(client.getPlayerId());
    if (!player)
        return "dead\n";
    int newRotation = (player->getRotation() == 0) ? 270 : (player->getRotation() - 90) % 360;

    player->setRotation(newRotation);
    _broadcastQueue->push(buildPipiMessage(*player));
    Logger::log("016", "Left : player turned left", {{"player_id", std::to_string(player->getId())}, {"rotation", std::to_string(player->getRotation())}});
    return "ok\n";
}
} // namespace zappy
