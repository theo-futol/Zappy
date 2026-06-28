#pragma once
#include <exception>
#include <string>
#include <source_location>

namespace zappy
{
    /// @brief Server-wide exception that captures both a message and the source
    ///        location where it was thrown, for clearer diagnostics.
    class ServerException : public std::exception
    {
    private:
        std::string _message;          ///< Human-readable error description.
        std::source_location _location; ///< Where the exception was constructed.
    public:
        /// @brief Builds the exception; the throw site is captured automatically
        ///        via the defaulted source_location argument.
        ServerException(const std::string &message, const std::source_location &loc = std::source_location::current()) : _message(message), _location(loc) {}

        /// @brief Returns the formatted message (text plus captured location).
        virtual const char *what() const noexcept override;
    };
}
