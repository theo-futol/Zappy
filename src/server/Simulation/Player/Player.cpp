#include "Player.hpp"

namespace zappy
{

Player::Player(int fd, const Team &team) : _fd(fd), _pos{0, 0}, rotation(Degrees::NORTH), _team(team), _isLeveling(false), _inventory(), _state(PlayerState::PENDING)
{
    _inventory.addItem(ItemType::FOOD, 10);
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

position Player::nextPosition(std::pair<int, int> mapSize) const
{
    position nextPos = _pos;

    switch (rotation)
    {
    case Degrees::NORTH: // Up
        nextPos.y -= 1;
        break;
    case Degrees::EAST: // Right
        nextPos.x += 1;
        break;
    case Degrees::SOUTH: // Down
        nextPos.y += 1;
        break;
    case Degrees::WEST: // Left
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

Team &Player::getTeam()
{
    return _team;
}

int Player::getFd() const
{
    return _fd;
}

const Team &Player::getTeam() const
{
    return _team;
}

void Player::changeState(PlayerState newState)
{
    _state = newState;
}

PlayerState Player::getState() const
{
    return _state;
}

void Player::writeToClient(const std::string &message) const
{
    if (_fd < 0)
        throw ServerException("Invalid file descriptor for player");
    ssize_t bytesSent = write(_fd, message.c_str(), message.size());
    if (bytesSent < 0)
        throw ServerException("Failed to send message to client");
}

Degrees Player::getDirectionTo(const position &target, std::pair<int, int> mapSize) const
{
    if (_pos == target)
        return Degrees::NORTH;
    int dx = 0;
    int dy = 0;
    if (abs(target.x - _pos.x) < (mapSize.first - abs(target.x - _pos.x)))
        dx = target.x - _pos.x;
    else
        dx = mapSize.first - (target.x - _pos.x);
    if (abs(target.y - _pos.y) < (mapSize.second - abs(target.y - _pos.y)))
        dy = target.y - _pos.y;
    else
        dy = mapSize.second - (target.y - _pos.y);
    return static_cast<Degrees>(std::atan2(dy, dx) * 100);
}

void Player::setState(PlayerState newState)
{
    _state = newState;
}
} // namespace zappy
