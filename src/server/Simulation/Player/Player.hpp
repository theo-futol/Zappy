#pragma once
#include <string>

#include "../../ServerException/ServerException.hpp"
#include "Inventory/Inventory.hpp"
#include "Teams.hpp"

namespace zappy
{
class Player
{
  private:
    int _fd; // Needed to link the player to its client

    int _playerID;
    int _level = 1;
    position _pos;
    int rotation;
    Team _team;
    bool isLeveling;
    Inventory _inventory;

  public:
    Player(int playerID, int fd, const Team &team) : _fd(fd), _playerID(playerID), _pos{0, 0}, _team(team), isLeveling(false), _inventory()
    {
    }
    int getPlayerID() const;
    const Team &getTeam() const;
    Team &getTeam();
    const position &getPosition() const;
    int getRotation() const;
    int getLevel() const;
    Inventory &getInventory();

    void setRotation(int rotation);
    void setPosition(int x, int y, std::pair<int, int> mapSize);
    void move(std::pair<int, int> mapSize);
    void levelUp();
};
} // namespace zappy
