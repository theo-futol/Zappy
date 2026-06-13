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
} // namespace zappy
