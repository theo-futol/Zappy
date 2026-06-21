#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Forward(std::vector<std::string> args, Client &client)
{
    (void)args; // Unused parameter
    Player *player = _world->getPlayerByFd(client.getFd());
    if (!player)
        return "dead\n";

    player->move(_world->getMapSize());
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
    return "ok\n";
}
} // namespace zappy
