#include "Player.hpp"

namespace zappy
{

Player::Player(int fd, std::shared_ptr<Team> team) : _fd(fd), _pos{0, 0}, rotation(Degrees::NORTH), _team(team), _isLeveling(false), _inventory(), _state(PlayerState::PENDING)
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
    return *_team;
}

int Player::getFd() const
{
    return _fd;
}

const Team &Player::getTeam() const
{
    return *_team;
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

int Player::getDistanceTo(const position &target, std::pair<int, int> mapSize) const
{
    int dx = std::abs(target.x - _pos.x);
    int dy = std::abs(target.y - _pos.y);

    if (dx > (mapSize.first - dx))
        dx = mapSize.first - dx;
    if (dy > (mapSize.second - dy))
        dy = mapSize.second - dy;
    return std::sqrt((dx * dx) + (dy * dy));
}

int Player::getDistanceTo(const Player &target, std::pair<int, int> mapSize) const
{
    return getDistanceTo(target.getPosition(), mapSize);
}

Degrees Player::getDirectionTo(const position &target, std::pair<int, int> mapSize) const
{
    if (_pos == target)
        return Degrees::NORTH;
    int dx = target.x - _pos.x;
    int dy = target.y - _pos.y;
    if (std::abs(dx) > (mapSize.first - std::abs(dx)))
        dx = mapSize.first - dx;
    if (std::abs(dy) > (mapSize.second - std::abs(dy)))
        dy = mapSize.second - dy;
    return Direction::getNearestDirection(std::atan2(dy, dx) * 100);
}

void Player::setState(PlayerState newState)
{
    if (newState == PlayerState::DEAD)
        _team->removePlayer();
    _state = newState;
}

void Player::setFrozenUntil(std::chrono::steady_clock::time_point until)
{
    _frozenUntil = until;
}

bool Player::isFrozen() const
{
    return std::chrono::steady_clock::now() < _frozenUntil;
}

Degrees Player::getDirectionTo(const Player &target, std::pair<int, int> mapSize) const
{
    return getDirectionTo(target.getPosition(), mapSize);
}

void Player::addMessageToQueue(const std::string &message, int timeNeeded, int receiverFd)
{
    if (_messagesToSend.size() == 0)
        _messagesToSend.push_back(std::make_pair(message, std::vector<std::pair<std::pair<std::clock_t, int>, int>>{{std::make_pair(std::clock(), timeNeeded), receiverFd}}));
    else
    {
        for (auto &msg : _messagesToSend)
            if (msg.first == message)
            {
                msg.second.emplace_back(std::make_pair(std::clock(), timeNeeded), receiverFd);
                return;
            }
        _messagesToSend.push_back(std::make_pair(message, std::vector<std::pair<std::pair<std::clock_t, int>, int>>{{std::make_pair(std::clock(), timeNeeded), receiverFd}}));
    }
}

void Player::sendMessageToClient()
{
    if (_messagesToSend.size() == 0)
        return;
    std::clock_t currentTime = std::clock();
    for (auto &msg : _messagesToSend)
    {
        std::vector<std::pair<std::pair<std::clock_t, int>, int>> &times = msg.second;
        for (auto it = times.begin(); it != times.end(); it++)
            if (currentTime - it->first.first >= it->first.second * CLOCKS_PER_SEC / 1000)
            {
                if (it->second < 0)
                    throw ServerException("Invalid file descriptor for player");
                ssize_t bytesSent = write(it->second, msg.first.c_str(), msg.first.size());
                if (bytesSent < 0)
                    throw ServerException("Failed to send message to client");
                it = times.erase(it);
            }
            else
                break;
    }
}

void Player::sortQueueByTimeNeeded()
{
    for (auto &msg : _messagesToSend)
        std::sort(msg.second.begin(), msg.second.end(),
                  [](const std::pair<std::pair<std::clock_t, int>, int> &a, const std::pair<std::pair<std::clock_t, int>, int> &b) { return a.first.second < b.first.second; });
}

} // namespace zappy
