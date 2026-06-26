#include "Client.hpp"
#include <iostream>
#include <sys/socket.h>
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

void Client::write(const std::string &message) const
{
    if (_fd < 0)
    {
        std::cout << "[ERROR-440] " << _fd << std::endl; // TO DO
        return;
    }
    send(_fd, message.c_str(), message.size(), MSG_NOSIGNAL); // MSG_NOSIGNAL: never SIGPIPE if the client is disconnected
}

} // namespace zappy