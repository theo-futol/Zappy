#include "Player.hpp"
#include <algorithm>
#include <cmath>

namespace zappy
{

Player::Player(int id, int fd, std::shared_ptr<Team> team) : _id(id), _fd(fd), _pos{0, 0}, rotation(Degrees::NORTH), _team(team), _inventory(), _state(PlayerState::PENDING)
{
    _inventory.addItem(ItemType::FOOD, 10);
    Logger::log("040", "Player : created", {{"player_id", std::to_string(_id)}, {"team", _team->_name}, {"x", std::to_string(_pos.x)}, {"y", std::to_string(_pos.y)}});
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

int Player::getOrientation() const
{
    switch (rotation)
    {
    case Degrees::NORTH:
        return 1;
    case Degrees::EAST:
        return 2;
    case Degrees::SOUTH:
        return 3;
    case Degrees::WEST:
        return 4;
    default:
        return 1;
    }
}

void Player::setRotation(int rot)
{
    if (rot % 90 != 0 || rot < 0 || rot > 360)
        return;
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
        break;
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

int Player::getId() const
{
    return _id;
}

int Player::getFd() const
{
    return _fd;
}

const Team &Player::getTeam() const
{
    return *_team;
}

PlayerState Player::getState() const
{
    return _state;
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

Degrees Player::getDirectionTo(const std::pair<const position, int> target, std::pair<int, int> mapSize) const
{
    if (_pos == target.first)
        return Degrees::NONE;
    int dx = target.first.x - _pos.x;
    int dy = target.first.y - _pos.y;
    if (std::abs(dx) > (mapSize.first - std::abs(dx)))
        dx = (dx > 0 ? -1 : 1) * (mapSize.first - std::abs(dx));
    if (std::abs(dy) > (mapSize.second - std::abs(dy)))
        dy = (dy > 0 ? -1 : 1) * (mapSize.second - std::abs(dy));
    int bearing = (static_cast<int>(std::atan2(dx, -dy) * 180 / M_PI) + 360) % 360;
    if (rotation != NORTH || target.second != NORTH)
        bearing = (bearing - rotation + 360) % 360;
    return Direction::getNearestDirection(bearing);
}

void Player::setState(PlayerState newState)
{
    if (newState == PlayerState::DEAD && _state != PlayerState::DEAD)
        _team->removePlayer();
    _state = newState;
}

void Player::setFrozenUntil(std::chrono::steady_clock::time_point until)
{
    _frozenUntil = until;
    auto untilMs = std::chrono::duration_cast<std::chrono::milliseconds>(until - std::chrono::steady_clock::now()).count();
    Logger::log("042", "Player : frozen", {{"player_id", std::to_string(_id)}, {"until_ms", std::to_string(untilMs)}});
}

bool Player::isFrozen() const
{
    return std::chrono::steady_clock::now() < _frozenUntil;
}

Degrees Player::getDirectionTo(const Player &target, std::pair<int, int> mapSize) const
{
    return getDirectionTo(std::make_pair(target.getPosition(), target.getRotation()), mapSize);
}

void Player::addMessageToQueue(const std::string &message, int timeNeeded, int receiverFd)
{
    auto now = std::chrono::steady_clock::now();
    if (_messagesToSend.size() == 0)
        _messagesToSend.push_back(std::make_pair(message, std::vector<std::pair<std::pair<std::chrono::steady_clock::time_point, int>, int>>{{std::make_pair(now, timeNeeded), receiverFd}}));
    else
    {
        for (auto &msg : _messagesToSend)
            if (msg.first == message)
            {
                msg.second.emplace_back(std::make_pair(now, timeNeeded), receiverFd);
                return;
            }
        _messagesToSend.push_back(std::make_pair(message, std::vector<std::pair<std::pair<std::chrono::steady_clock::time_point, int>, int>>{{std::make_pair(now, timeNeeded), receiverFd}}));
    }
}

void Player::sendMessageToClient(std::chrono::steady_clock::time_point currentTime, std::vector<std::unique_ptr<Client>> &clients)
{
    if (_messagesToSend.size() == 0)
        return;
    for (auto msgIt = _messagesToSend.begin(); msgIt != _messagesToSend.end();)
    {
        std::vector<std::pair<std::pair<std::chrono::steady_clock::time_point, int>, int>> &times = msgIt->second;
        // times is sorted by timeNeeded, so stop at the first entry that is not ready yet.
        auto it = times.begin();
        for (; it != times.end(); it = times.erase(it))
        {
            if (currentTime - it->first.first < std::chrono::milliseconds(it->first.second))
                break;
            if (it->second >= 0)
                // Find the client with the matching fd and send the message
                for (const auto &client : clients)
                    if (client->getFd() == it->second)
                    {
                        client->write(msgIt->first);
                        break;
                    }
        }
        if (times.empty())
            msgIt = _messagesToSend.erase(msgIt);
        else
            ++msgIt;
    }
}

void Player::sortQueueByTimeNeeded()
{
    for (auto &msg : _messagesToSend)
        std::sort(msg.second.begin(), msg.second.end(),
                  [](const auto &a, const auto &b) { return a.first.second < b.first.second; });
}

} // namespace zappy
