#pragma once
#include <string>

#include "../../ServerException/ServerException.hpp"
#include "Inventory/Inventory.hpp"
#include "Teams.hpp"
#include "../Utils.hpp"

#define NORTH 0
#define EAST 90
#define SOUTH 180
#define WEST 270

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

  public:
    Player(int fd, const Team &team) : _fd(fd),  _pos{0, 0}, rotation(NORTH), _team(team), _isLeveling(false), _inventory(), _state(PlayerState::PENDING)
    {
    }
    const Team &getTeam() const;
    Team &getTeam();

    const position &getPosition() const;
    int getRotation() const;
    int getLevel() const;
    Inventory &getInventory();

    int getFd() const;
    void setRotation(int rotation);
    void setPosition(int x, int y, std::pair<int, int> mapSize);
    position nextPosition(std::pair<int, int> mapSize) const;
    void move(std::pair<int, int> mapSize);
    void levelUp();
    void changeState(PlayerState newState);
    PlayerState getState() const;
    void writeToClient(const std::string &message) const;
};
} // namespace zappy
