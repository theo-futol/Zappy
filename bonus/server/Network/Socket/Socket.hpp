#pragma once

namespace zappy
{
    /// @brief Thin RAII wrapper around a single BSD socket file descriptor.
    ///
    /// Mirrors the raw socket() / bind() / listen() / accept() calls but owns the
    /// descriptor and closes it on destruction. Used for the server's listening socket.
    class Socket
    {
    private:
        int _socket = -1; ///< Owned descriptor, -1 when none.

    public:
        /// @brief Creates an empty wrapper holding no descriptor.
        Socket() = default;

        /// @brief Adopts an existing, already-open descriptor.
        Socket(int socket) : _socket(socket) {};

        /// @brief Closes the owned descriptor if one is held.
        ~Socket();

        /// @brief Opens a new socket (wraps ::socket). @return false on failure.
        bool create(int domain, int type, int protocol);

        /// @brief Binds the socket to the given port on all interfaces. @return false on failure.
        bool bind(int port);

        /// @brief Marks the socket as passive/listening. @return false on failure.
        bool listen();

        /// @brief Accepts a pending connection. @return the new client fd, or -1 on failure.
        int accept();

        /// @brief Returns the owned descriptor.
        int getFd() const;
        int getFd();

        /// @brief Replaces the owned descriptor.
        void setSocket(int socket);
    };
}
