#pragma once
#include <string>
#include "ClientType.hpp"

namespace zappy
{
    /// @brief A single connected socket: its file descriptor, its role, and its
    ///        pending I/O buffer. One Client exists per accepted connection,
    ///        regardless of whether it turns out to be a graphic or AI client.
    class Client
    {
        private:
            int _fd;             // OS socket descriptor identifying this connection
            ClientType _type;    // Role of the connection (graphic / AI / dead / not yet known)
            std::string _buffer; // Bytes received but not yet consumed as a full command

        public:
            /// @brief Wraps an already-accepted socket. A fresh client starts UNKNOWN
            ///        until its first message reveals whether it is graphic or AI.
            Client(int fd, ClientType type = ClientType::UNKNOWN) : _fd(fd), _type(type), _buffer() {}

            // Network related tasks

            /// @brief Returns the socket descriptor, used to read/write and to poll this client.
            int getFd() const;

            /// @brief Returns the connection's current role.
            ClientType getType() const;

            /// @brief Sets the connection's role, e.g. once it has been identified or marked dead.
            void setType(ClientType type);

            /// @brief Read-only view of the pending receive buffer.
            const std::string &getBuffer() const;

            /// @brief Mutable access to the pending receive buffer, so callers can
            ///        append incoming bytes or erase a command once it is parsed.
            std::string &getBuffer();

            /// @brief Replaces the entire pending buffer.
            void setBuffer(const std::string &buffer);
    };
} // namespace zappy
