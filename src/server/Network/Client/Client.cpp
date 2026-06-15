#include "Client.hpp"

namespace zappy
{
int Client::getFd() const
{
    return _fd;
}

const std::string &Client::getBuffer() const
{
    return _buffer;
}

std::string &Client::getBuffer()
{
    return _buffer;
}

ClientType Client::getType() const
{
    return _type;
}

void Client::setType(ClientType type)
{
    _type = type;
}

void Client::setBuffer(const std::string &buffer)
{
    _buffer = buffer;
}

int Client::getPlayerID() const
{
    return _playerID;
}

void Client::setPlayerID(int playerID)
{
    this->_playerID = playerID;
}
} // namespace zappy