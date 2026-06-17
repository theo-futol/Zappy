#pragma once
#include <poll.h>
#include <memory>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <string.h>
#include <algorithm>

#include "../Client/Client.hpp"
#include "../Socket/Socket.hpp"
#include "CommandParser/CommandParser.hpp"
#include "../../Simulation/World/World.hpp"
#include "../../ServerException/ServerException.hpp"

/// @brief Manages all TCP client connections and drives the main network event loop.
///
/// ClientHandler owns the listening socket, the list of connected clients, and one
/// CommandParser per client. It multiplexes I/O with poll(), forwards incoming data
/// to each parser, and triggers periodic world resource updates.
namespace zappy
{
    class ClientHandler
    {
        private:
            Socket _tcpSocket;                                          ///< Listening TCP socket.
            int _f;                                                     ///< Reciprocal of the time unit (from -f arg).
            std::vector<pollfd> _fds;                                   ///< Monitored file descriptors (index 0 = listening socket).
            std::vector<std::unique_ptr<Client>> _clients;              ///< Connected clients.
            std::unordered_map<int, std::unique_ptr<CommandParser>> _parsers; ///< One parser per client fd.
            std::chrono::steady_clock::time_point _lastResourceUpdate;  ///< Timestamp of the last world resource tick.
            World *_world;                                              ///< Non-owning pointer to the simulation world.
            bool * _serverIsRunning;                                    ///< Flag to control the main loop.
            std::queue <std::string> _broadcastQueue;                   ///< Queue of pending broadcast messages to send to all the clients.
            /// @brief Iterates connected clients, handles disconnections and dispatches incoming commands.
            void clientEventHandling();
            void broadcastGuiInfo();

        public:
            /// @brief Constructs a handler, binds and listens on the given port.
            /// @param port                 TCP port to listen on.
            /// @param initialClientCapacity Initial capacity hint for the fd and client vectors.
            /// @param f                    Reciprocal of the time unit (default: 100).
            /// @param world                Non-owning pointer to the simulation world.
            /// @param serverIsRunning      Pointer to the flag controlling the main loop.
            ClientHandler(int port, int initialClientCapacity, int f = 100, World *world = nullptr, bool *serverIsRunning = nullptr);

            ~ClientHandler();

            /// @brief Runs the blocking event loop: polls for I/O, accepts new clients,
            ///        dispatches commands, and triggers world resource updates every TIMEOUT ms.
            /// @throws ServerException if poll() fails with an unrecoverable error.
            void handleClients(void);

            /// @brief Accepts a new incoming connection, sends the WELCOME handshake,
            ///        and registers the client and its parser.
            void addClient();

            /// @brief Removes a client, its parser, and its fd from all internal structures.
            /// @param fd File descriptor of the client to remove.
            void removeClient(int fd);

            /// @brief Looks up a client by its file descriptor.
            /// @param fd File descriptor to search for.
            /// @return Pointer to the matching Client, or nullptr if not found.
            Client *getClientByFd(int fd) const;
    };
} // namespace zappy
