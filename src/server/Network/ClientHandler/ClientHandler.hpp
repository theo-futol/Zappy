#pragma once
#include <poll.h>
#include <memory>
#include <vector>
#include <chrono>
#include <unordered_map>

#include "../Client/Client.hpp"
#include "../Socket/Socket.hpp"
#include "CommandParser/CommandParser.hpp"
#include "../../Simulation/World/World.hpp"

namespace zappy
{
    class ClientHandler
    {
        private:
            Socket _tcpSocket;
            int _f;
            std::vector<pollfd> _fds;
            std::vector<std::unique_ptr<Client>> _clients;
            std::unordered_map<int, std::unique_ptr<CommandParser>> _parsers;
            std::chrono::steady_clock::time_point _lastResourceUpdate;
        public:
            ClientHandler();
            ClientHandler(int port, int initialClientCapacity, int f = 100);
            ~ClientHandler();

            /// @brief Main loop for handling client connections and events using poll().
            void handleClients(void);

            /// @brief Accepts a new connection, sends WELCOME, and registers the client.
            void addClient();

            /// @brief Removes a client and its parser from the handler.
            /// @param fd The file descriptor of the client to remove.
            void removeClient(int fd);

            // HELPER FUNCTIONS

            /// @brief Retrieves a pointer to the Client object associated with the given file descriptor.
            /// @param fd The file descriptor of the client.
            /// @return A pointer to the Client object, or nullptr if not found.
            Client *getClientByFd(int fd) const;
    };
} // namespace zappy
