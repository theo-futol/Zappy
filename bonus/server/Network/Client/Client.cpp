#include "Client.hpp"
#include <unistd.h>
#include <utility>

namespace zappy
{
Client::~Client()
{
    if (_fd >= 0)
        close(_fd);
}

int Client::getFd() const
{
    return _fd;
}

int Client::getPlayerId() const
{
    return _playerId;
}

void Client::setPlayerId(int playerId)
{
    _playerId = playerId;
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

} // namespace zappy