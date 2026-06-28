#include "Network/socket/TcpSocket.hpp"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <netdb.h>
#include <sys/socket.h>

namespace Zappy
{

TcpSocket::TcpSocket() : _fd(-1)
{
    _fd = socket(AF_INET, SOCK_STREAM, 0);
    if (_fd == -1)
    {
        throw TcpSocketException("Failed to create socket");
    }
}

TcpSocket::~TcpSocket()
{
    if (_fd != -1)
    {
        ::close(_fd);
    }
}

void TcpSocket::connect(const std::string &host, int port)
{
    addrinfo *res = nullptr;
    addrinfo base_infos = {};
    int flags = 0;
    int ai_res = 0;

    base_infos.ai_family = AF_INET;
    base_infos.ai_socktype = SOCK_STREAM;
    ai_res = getaddrinfo(host.c_str(), std::to_string(port).c_str(), &base_infos, &res);
    if (ai_res != 0)
    {
        throw TcpSocketException("Failed to get address info: getaddrinfo: " + std::string(gai_strerror(ai_res)));
    }
    if (::connect(_fd, res->ai_addr, res->ai_addrlen) == -1)
    {
        freeaddrinfo(res);
        throw TcpSocketException("Failed to connect to server: connect: " + std::string(strerror(errno)));
    }
    freeaddrinfo(res);
    flags = fcntl(_fd, F_GETFL, 0);
    fcntl(_fd, F_SETFL, flags | O_NONBLOCK);
}

void TcpSocket::send(const std::string &data)
{
    std::size_t sent = 0;

    while (sent < data.size())
    {
        ssize_t n = ::send(_fd, data.data() + sent, data.size() - sent, MSG_NOSIGNAL);
        if (n < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            throw TcpSocketException("send: " + std::string(strerror(errno)));
        }
        sent += n;
    }
}

std::string TcpSocket::receive()
{
    char buffer[1024] = {0};
    ssize_t size_read = ::recv(_fd, buffer, sizeof(buffer) - 1, 0);

    if (size_read < 0)
    {
        if (errno == EWOULDBLOCK || errno == EAGAIN)
            return "";
        throw TcpSocketException("Failed to receive data from server: recv: " + std::string(strerror(errno)));
    }
    if (size_read == 0)
    {
        throw TcpSocketException("Server closed the connection");
    }
    return std::string(buffer, static_cast<std::size_t>(size_read));
}

} // namespace Zappy
