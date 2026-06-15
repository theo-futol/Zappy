#pragma once
#include <sys/socket.h>
#include <sys/poll.h>
#include <unistd.h>
#include <iostream>
#include <arpa/inet.h>

namespace zappy
{
    class Socket
    {
    private:
        int _socket = -1;

    public:
        Socket() = default;
        Socket(int socket) : _socket(socket) {};
        ~Socket();
        bool create(int domain, int type, int protocol);
        bool bind(int port);
        bool listen();
        int accept();
        int getFd() const;
        int getFd();
        void setSocket(int socket);
    };
}
