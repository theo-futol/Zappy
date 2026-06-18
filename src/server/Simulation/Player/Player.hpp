#pragma once
#include <string>
#include <math.h>
#include <algorithm>
#include <chrono>

#include "../../ServerException/ServerException.hpp"
#include "Inventory/Inventory.hpp"
#include "Teams.hpp"
#include "../Utils.hpp"

#define BROADCAST_MESSAGE_TIME_PER_TILE 7

namespace zappy
{
class Player
{
  private:
    int _fd; // Needed to link the player to its client

    int _level = 1;
    position _pos;
    int rotation;
    Team _team;
    bool _isLeveling;
    Inventory _inventory;
    PlayerState _state;
    std::vector<std::pair<std::string, std::vector<std::pair<std::pair<std::clock_t, int>, int>>>> _messagesToSend; // <<message, <<clock, timeNeeded>, receiverFd>, <clock, timeNeeded>, receiverFd>>, <message, <<clock, timeNeeded>, receiverFd>>>

  public:
    Player(int fd, const Team &team) : _fd(fd),  _pos{0, 0}, rotation(Degrees::NORTH), _team(team), _isLeveling(false), _inventory(), _state(PlayerState::PENDING)
    {
    }
    const Team &getTeam() const;
    Team &getTeam();

    const position &getPosition() const;
    int getRotation() const;
    int getLevel() const;
    Inventory &getInventory();
    PlayerState getState() const;
    int getFd() const;

    position nextPosition(std::pair<int, int> mapSize) const;
    void setRotation(int rotation);
    void setPosition(int x, int y, std::pair<int, int> mapSize);
    void changeState(PlayerState newState);
    void move(std::pair<int, int> mapSize);
    void levelUp();
    void writeToClient(const std::string &message) const;
    Degrees getDirectionTo(const position &target, std::pair<int, int> mapSize) const;
    Degrees getDirectionTo(const Player &target, std::pair<int, int> mapSize) const;
    int getDistanceTo(const position &target, std::pair<int, int> mapSize) const;
    int getDistanceTo(const Player &target, std::pair<int, int> mapSize) const;
    // Broadcast a message to all players in the world, except the sender
    // TO DO : Use a class to represent the queue
    void sortQueueByTimeNeeded();
    void addMessageToQueue(const std::string &message, int timeNeeded, int receiverFd);
    void sendMessageToClient();
  };
} // namespace zappy
