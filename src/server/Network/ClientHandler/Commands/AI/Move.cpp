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

    player->setRotation((player->getRotation() + 90) % 360);
    return "ok\n";
}
std::string Commands::Left(std::vector<std::string> args, Client &client)
{
    (void)args; // Unused parameter
    Player *player = _world->getPlayerByFd(client.getFd());
    if (!player)
        return "dead\n";

    player->setRotation((player->getRotation() - 90) % 360);
    return "ok\n";
}
} // namespace zappy
