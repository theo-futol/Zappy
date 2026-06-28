#include "../Socket/Socket.hpp"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

namespace zappy
{

Socket::~Socket()
{
    if (_socket != -1)
        close(_socket);
}

bool Socket::create(int domain, int type, int protocol)
{
    _socket = socket(domain, type, protocol);
    return _socket != -1;
}

bool Socket::bind(int port)
{
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    return ::bind(_socket, (struct sockaddr *)&address, sizeof(address)) != -1;
}

bool Socket::listen()
{
    return ::listen(_socket, SOMAXCONN) != -1;
}

int Socket::accept()
{
    return ::accept(_socket, nullptr, nullptr);
}

int Socket::getFd() const
{
    return _socket;
}

int Socket::getFd()
{
    return _socket;
}

void Socket::setSocket(int socket)
{
    _socket = socket;
}

} // namespace zappy