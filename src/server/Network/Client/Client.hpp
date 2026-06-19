#pragma once
#include <string>
#include "ClientType.hpp"

namespace zappy
{
    class Client
    {
        private:
            int _fd;
            ClientType _type;
            std::string _buffer;
            public:
            Client(int fd, ClientType type = ClientType::UNKNOWN) : _fd(fd), _type(type), _buffer() {}
            // Network related tasks
            int getFd() const;
            ClientType getType() const;
            void setType(ClientType type);
            const std::string &getBuffer() const;
            std::string &getBuffer();
            void setBuffer(const std::string &buffer);
    };
} // namespace zappy
