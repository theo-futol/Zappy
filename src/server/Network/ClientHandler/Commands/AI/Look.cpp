#include "../Commands.hpp"

namespace zappy
{
std::string Commands::Look(std::vector<std::string> args, Client &client)
{
    (void)args; // Unused parameter
    std::string buffer = "[";
    Player *player = _world->getPlayerByFd(client.getFd());
    if (!player)
        return "ko\n";
    std::pair<int, int> mapSize = _world->getMapSize();
    position pos = player->getPosition();
    int rotation = player->getRotation();

    for (int depth = 0; depth <= player->getLevel(); depth++)
    {
        for (int lateral = -depth; lateral <= depth; lateral++)
        {
            position tilePos = pos;

            switch (rotation)
            {
            case NORTH:
                tilePos.x += lateral;
                tilePos.y -= depth;
                break;
            case EAST:
                tilePos.x += depth;
                tilePos.y += lateral;
                break;
            case SOUTH:
                tilePos.x -= lateral;
                tilePos.y += depth;
                break;
            case WEST:
                tilePos.x -= depth;
                tilePos.y -= lateral;
                break;
            default:
                throw ServerException("Invalid rotation value");
            }

            tilePos.x = (tilePos.x + mapSize.first) % mapSize.first;
            tilePos.y = (tilePos.y + mapSize.second) % mapSize.second;

            tile *currentTile = _world->getTileAt(tilePos);
            for (const auto &item : currentTile->_items)
                buffer += itemTypeToString(item.first) + ":" + std::to_string(item.second) + " ";
            for (int playerIndex = 0; playerIndex < static_cast<int>(currentTile->_players.size()); ++playerIndex)
                buffer += "player ";
            if (!(depth == player->getLevel() && lateral == depth))
                buffer += ", ";
        }
    }
    buffer += "]";
    Logger::log("017", "Look : response sent", {{"player_id", std::to_string(player->getFd())}, {"response", buffer}});
    return buffer + "\n";
}
} // namespace zappy
