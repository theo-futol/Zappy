#pragma once
#include <exception>
#include <iostream>
#include <string>
#include <source_location>

namespace zappy
{
    class ServerException : public std::exception
    {
    private:
        std::string _message;
        std::source_location _location;
    public:
        ServerException(const std::string &message, const std::source_location &loc = std::source_location::current()) : _message(message), _location(loc) {}
        virtual const char *what() const noexcept override;
    };
}
