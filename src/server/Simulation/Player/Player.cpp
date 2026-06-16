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

void Player::move(std::pair<int, int> mapSize)
{
    switch (rotation)
    {
    case 0: // Up
        setPosition(_pos.x, _pos.y - 1, mapSize);
        break;
    case 90: // Right
        setPosition(_pos.x + 1, _pos.y, mapSize);
        break;
    case 180: // Down
        setPosition(_pos.x, _pos.y + 1, mapSize);
        break;
    case 270: // Left
        setPosition(_pos.x - 1, _pos.y, mapSize);
        break;
    default:
        throw ServerException("Invalid rotation value for movement");
    }
}

Inventory &Player::getInventory()
{
    return _inventory;
}

Team &Player::getTeam()
{
    return _team;
}

const Team &Player::getTeam() const
{
    return _team;
}

} // namespace zappy
