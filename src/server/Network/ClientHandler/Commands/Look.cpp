#include "Commands.hpp"

namespace zappy
{
std::string Commands::Look(std::vector<std::string> args, Client &client)
{
    (void)args; // Unused parameter
    std::string buffer = "[";
    Player *player = _world->getPlayerByFd(client.getFd());

    for (int i = 0; i < player->getLevel(); i++)
    {
        position pos = player->getPosition();
        int rotation = player->getRotation();
        std::pair<int, int> mapSize = _world->getMapSize();

        for (int j = 0; j < (3 * (i + 1) - i); j++)
        {
            position tilePos = pos;

            switch (rotation)
            {
            case NORTH:
                tilePos.x += j - i;
                tilePos.y -= i;
                break;
            case EAST:
                tilePos.x += i;
                tilePos.y += j - i;
                break;
            case SOUTH:
                tilePos.x += j - i;
                tilePos.y += i;
                break;
            case WEST:
                tilePos.x -= i;
                tilePos.y += j - i;
                break;
            default:
                throw ServerException("Invalid rotation value");
            }

            if (tilePos.x < 0)
                tilePos.x = mapSize.first - 1;
            else if (tilePos.x >= mapSize.first)
                tilePos.x = 0;
            if (tilePos.y < 0)
                tilePos.y = mapSize.second - 1;
            else if (tilePos.y >= mapSize.second)
                tilePos.y = 0;

            tile *currentTile = _world->getTileAt(tilePos);
            for (const auto &item : currentTile->_items)
                buffer += itemTypeToString(item.first) + " " + std::to_string(item.second) + " "; // TO DO : used count instead of multiply the item name
            if (!currentTile->_players.empty())
                buffer += "player " + std::to_string(currentTile->_players.size() - (tilePos == player->getPosition())) + " ";
        }
        if (i != player->getLevel() - 1)
            buffer += ",";
    }
    return buffer + "]";
}
} // namespace zappy
