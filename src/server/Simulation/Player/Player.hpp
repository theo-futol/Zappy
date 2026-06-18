#pragma once
#include <string>
#include <math.h>

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
    Player(int fd, const Team &team);
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
    void writeToClient(const std::string &message) const; // TO DO : Use the client instead of the fd to write to the client
    Degrees getDirectionTo(const position &target, std::pair<int, int> mapSize) const;
    void setState(PlayerState newState);
};
} // namespace zappy
