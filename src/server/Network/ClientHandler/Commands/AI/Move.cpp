#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Forward(std::vector<std::string> args, Client &client)
{
    (void)args; // Unused parameter
    Player *player = _world->getPlayerByFd(client.getFd());
    if (!player)
    {
        Logger::log("8424", "Forward : failed", {{"player_id", std::to_string(client.getFd())}});
        return "dead\n";
    }

    position oldPos = player->getPosition();
    player->move(_world->getMapSize());
    _world->removePlayerFromTile(player, oldPos);
    _world->addPlayerToTile(player, player->getPosition());
    position newPos = player->getPosition();
    Logger::log("014", "Forward : player moved", {{"player_id", std::to_string(player->getFd())}, {"x", std::to_string(newPos.x)}, {"y", std::to_string(newPos.y)}});
    return "ok\n";
}
std::string Commands::Right(std::vector<std::string> args, Client &client)
{
    (void)args; // Unused parameter
    Player *player = _world->getPlayerByFd(client.getFd());
    if (!player)
        return "dead\n";
    int newRotation = (player->getRotation() + 90) % 360;

    player->setRotation(newRotation);
    Logger::log("015", "Right : player turned right", {{"player_id", std::to_string(player->getFd())}, {"rotation", std::to_string(player->getRotation())}});
    return "ok\n";
}
std::string Commands::Left(std::vector<std::string> args, Client &client)
{
    (void)args; // Unused parameter
    Player *player = _world->getPlayerByFd(client.getFd());
    if (!player)
        return "dead\n";
    int newRotation = (player->getRotation() == 0) ? 270 : (player->getRotation() - 90) % 360;

    player->setRotation(newRotation);
    Logger::log("016", "Left : player turned left", {{"player_id", std::to_string(player->getFd())}, {"rotation", std::to_string(player->getRotation())}});
    return "ok\n";
}
} // namespace zappy
