#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Fork(std::vector<std::string> args, Client &client)
{
    (void)args; // Unused parameter
    Player *player = _world->getPlayerByFd(client.getFd());
    if (!player)
        return "ko\n";
    position playerPos = player->getPosition();

    _world->setTileAt(playerPos, ItemType::EGG, 1);
    player->getTeam().addEgg(playerPos);
    return "ok\n";
}

bool Commands::beginIncantation(Client &client, std::chrono::steady_clock::time_point endTime)
{
    Player *initiator = _world->getPlayerByFd(client.getFd());
    if (!initiator)
        return false;
    position pos = initiator->getPosition();
    int level = initiator->getLevel();
    tile *tilePtr = _world->getTileAt(pos);

    // Only one ritual may run on a tile at a time.
    if (!tilePtr || tilePtr->_incantationInProgress || !_world->isIncantationValid(pos.x, pos.y, level))
    {
        initiator->writeToClient("ko\n");
        return false;
    }
    tilePtr->_incantationInProgress = true;

    std::vector<Player *> participants = _world->getPlayersOnTileAtLevel(pos.x, pos.y, level);
    std::string pic = "pic " + std::to_string(pos.x) + " " + std::to_string(pos.y) + " " + std::to_string(level);
    for (const Player *participant : participants)
        pic += " " + std::to_string(participant->getFd());
    _broadcastQueue->push(pic + "\n");

    for (Player *participant : participants)
    {
        participant->writeToClient("Elevation underway\n");
        participant->setFrozenUntil(endTime);
    }
    return true;
}

std::string Commands::Incantation(std::vector<std::string> args, Client &client)
{
    (void)args; // Unused parameter
    Player *initiator = _world->getPlayerByFd(client.getFd());
    if (!initiator)
        return "ko\n";
    position pos = initiator->getPosition();
    int level = initiator->getLevel();
    std::string pieHeader = "pie " + std::to_string(pos.x) + " " + std::to_string(pos.y) + " ";
    tile *tilePtr = _world->getTileAt(pos);

    if (tilePtr)
        tilePtr->_incantationInProgress = false;

    if (!_world->isIncantationValid(pos.x, pos.y, level))
    {
        _broadcastQueue->push(pieHeader + "0\n");
        return "ko\n";
    }

    std::vector<Player *> participants = _world->getPlayersOnTileAtLevel(pos.x, pos.y, level);
    _world->removeIncantationStones(pos.x, pos.y, level);
    int new_level = 0;
    for (Player *participant : participants)
    {
        participant->levelUp();
        _broadcastQueue->push("plv " + std::to_string(participant->getFd()) + " " + std::to_string(participant->getLevel()) + "\n");
        participant->writeToClient("Current level: " + std::to_string(participant->getLevel()) + "\n");
        new_level = participant->getLevel();
    }
    std::cout << "Player " << initiator->getFd() << " attained a new level : " << new_level << std::endl;
    _broadcastQueue->push(pieHeader + "1\n");
    return "";
}
} // namespace zappy
