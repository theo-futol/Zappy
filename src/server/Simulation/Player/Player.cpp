#include "Player.hpp"

namespace zappy
{
int Player::getPlayerID() const
{
    return _playerID;
}

const position &Player::getPosition() const
{
    return _pos;
}

Team Player::getTeam()
{
    return _team;
}

Team Player::getTeam() const
{
    return _team;
}

int Player::getLevel() const
{
    return _level;
}

void Player::levelUp()
{
    _level++;
}

int Player::getRotation() const
{
    return rotation;
}

void Player::setRotation(int rot)
{
    if (rot % 90 != 0 || rot < 0 || rot > 360)
        throw ServerException("Rotation must be between 0 and 360 degrees");
    rotation = rot;
}

void Player::setPosition(int x, int y, std::pair<int, int> mapSize)
{
    // Map is a toroidal grid, so we wrap around if the player goes out of bounds
    if (x < 0)
        x = mapSize.first - 1;
    else if (x >= mapSize.first)
        x = 0;
    if (y < 0)
        y = mapSize.second - 1;
    else if (y >= mapSize.second)
        y = 0;
    _pos.x = x;
    _pos.y = y;
}

position Player::nextPosition(std::pair<int, int> mapSize) const
{
    position nextPos = _pos;

    switch (rotation)
    {
    case 0: // Up
        nextPos.y -= 1;
        break;
    case 90: // Right
        nextPos.x += 1;
        break;
    case 180: // Down
        nextPos.y += 1;
        break;
    case 270: // Left
        nextPos.x -= 1;
        break;
    default:
        throw ServerException("Invalid rotation value for movement");
    }

    // Wrap around if the player goes out of bounds
    if (nextPos.x < 0)
        nextPos.x = mapSize.first - 1;
    else if (nextPos.x >= mapSize.first)
        nextPos.x = 0;

    if (nextPos.y < 0)
        nextPos.y = mapSize.second - 1;
    else if (nextPos.y >= mapSize.second)
        nextPos.y = 0;

    return nextPos;
}

void Player::move(std::pair<int, int> mapSize)
{
    position nextPos = nextPosition(mapSize);

    setPosition(nextPos.x, nextPos.y, mapSize);
}

Inventory &Player::getInventory()
{
    return _inventory;
}

void Player::writeToClient(const std::string &message) const
{
    if (_fd < 0)
        throw ServerException("Invalid file descriptor for player");
    ssize_t bytesSent = write(_fd, message.c_str(), message.size());
    if (bytesSent < 0)
        throw ServerException("Failed to send message to client");

} // namespace zappy
