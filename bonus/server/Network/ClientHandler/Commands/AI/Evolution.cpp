#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Fork(std::vector<std::string>, Client &client, std::vector<std::unique_ptr<Client>> &)
{
    Player *player = _world->getPlayerById(client.getPlayerId());
    if (!player)
    {
        Logger::log("8420", "Fork : failed", {{"player_id", std::to_string(client.getPlayerId())}});
        return "ko\n";
    }
    position playerPos = player->getPosition();

    _world->setTileAt(playerPos, ItemType::EGG, 1);
    player->getTeam().addEgg(playerPos);
    Logger::log("010", "Fork : new egg laid",
                {{"player_id", std::to_string(player->getId())}, {"team", player->getTeam()._name}, {"x", std::to_string(playerPos.x)}, {"y", std::to_string(playerPos.y)}});
    return "ok\n";
}

bool Commands::beginIncantation(Client &client, std::chrono::steady_clock::time_point endTime, std::vector<std::unique_ptr<Client>> &clients)
{
    Player *initiator = _world->getPlayerById(client.getPlayerId());
    if (!initiator)
        return false;
    position pos = initiator->getPosition();
    int level = initiator->getLevel();
    tile *tilePtr = _world->getTileAt(pos);

    // Only one ritual may run on a tile at a time.
    if (!tilePtr || tilePtr->_incantationInProgress || !_world->isIncantationValid(pos.x, pos.y, level))
    {
        Logger::log("8422", "Incantation : failed, conditions not met at start",
                    {{"player_id", std::to_string(initiator->getId())}, {"x", std::to_string(pos.x)}, {"y", std::to_string(pos.y)}});
        client.write("ko\n");
        return false;
    }
    tilePtr->_incantationInProgress = true;

    std::vector<Player *> participants = _world->getPlayersOnTileAtLevel(pos.x, pos.y, level);
    std::string pic = "pic " + std::to_string(pos.x) + " " + std::to_string(pos.y) + " " + std::to_string(level);
    for (const Player *participant : participants)
        pic += " " + std::to_string(participant->getId());
    _broadcastQueue->push(pic + "\n");

    for (Player *participant : participants)
    {
        Client *participantClient =
            find_if(clients.begin(), clients.end(), [participant](const std::unique_ptr<Client> &c) { return c->getPlayerId() == participant->getId(); })->get();
        participantClient->write("Elevation underway\n");
        participant->setFrozenUntil(endTime);
    }
    Logger::log("012", "Incantation : started",
                {{"player_id", std::to_string(initiator->getId())}, {"level", std::to_string(level)}, {"x", std::to_string(pos.x)}, {"y", std::to_string(pos.y)}});
    return true;
}

std::string Commands::Incantation(std::vector<std::string>, Client &client, std::vector<std::unique_ptr<Client>> &clients)
{
    Player *initiator = _world->getPlayerById(client.getPlayerId());
    if (!initiator)
        return "ko\n";
    position pos = initiator->getPosition();
    int level = initiator->getLevel();
    std::string pieHeader = "pie " + std::to_string(pos.x) + " " + std::to_string(pos.y) + " ";
    tile *tilePtr = _world->getTileAt(pos);

    if (tilePtr)
        tilePtr->_incantationInProgress = false;

    std::vector<Player *> participants = _world->getPlayersOnTileAtLevel(pos.x, pos.y, level);
    for (Player *participant : participants)
        Logger::log("043", "Player : unfrozen", {{"player_id", std::to_string(participant->getId())}});

    if (!_world->isIncantationValid(pos.x, pos.y, level))
    {
        Logger::log("8423", "Incantation : failed, conditions not met at end",
                    {{"player_id", std::to_string(initiator->getId())}, {"x", std::to_string(pos.x)}, {"y", std::to_string(pos.y)}});
        for (Player *participant : participants)
        {
            Client *participantClient =
                find_if(clients.begin(), clients.end(), [participant](const std::unique_ptr<Client> &c) { return c->getPlayerId() == participant->getId(); })->get();
            participantClient->write("ko\n");
        }
        _broadcastQueue->push(pieHeader + "0\n");
        return "";
    }

    _world->removeIncantationStones(pos.x, pos.y, level);
    int new_level = 0;
    for (Player *participant : participants)
    {
        participant->levelUp();
        Logger::log("041", "Player : level up", {{"player_id", std::to_string(participant->getId())}, {"level", std::to_string(participant->getLevel())}});
        _broadcastQueue->push("plv " + std::to_string(participant->getId()) + " " + std::to_string(participant->getLevel()) + "\n");
        _broadcastQueue->push(buildPipiMessage(*participant));
        Client *participantClient =
            find_if(clients.begin(), clients.end(), [participant](const std::unique_ptr<Client> &c) { return c->getPlayerId() == participant->getId(); })->get();
        participantClient->write("Current level: " + std::to_string(participant->getLevel()) + "\n");
        new_level = participant->getLevel();
    }
    Logger::log("013", "Incantation : completed",
                {{"player_id", std::to_string(initiator->getId())}, {"level", std::to_string(new_level)}, {"x", std::to_string(pos.x)}, {"y", std::to_string(pos.y)}});
    _broadcastQueue->push(pieHeader + "1\n");
    return "";
}
} // namespace zappy
