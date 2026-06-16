#pragma once
#include <string>

#include "../../ServerException/ServerException.hpp"
#include "Inventory/Inventory.hpp"
#include "Teams.hpp"
#include "../Utils.hpp"

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
    Player(int fd, const Team &team) : _fd(fd),  _pos{0, 0}, _team(team), _isLeveling(false), _inventory(), _state(PlayerState::PENDING)
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
    void move(std::pair<int, int> mapSize);
    void levelUp();
};
} // namespace zappy
