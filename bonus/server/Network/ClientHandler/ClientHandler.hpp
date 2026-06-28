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

/// @brief Manages all TCP client connections and drives the main network event loop.
namespace zappy
{
    class ClientHandler
    {
    protected:
        Socket _tcpSocket;
        std::vector<pollfd> _fds;
        std::vector<std::unique_ptr<Client>> _clients;
        std::unordered_map<int, std::unique_ptr<CommandParser>> _parsers;
        std::chrono::steady_clock::time_point _lastResourceUpdate;
        std::chrono::steady_clock::time_point _lastFoodUpdate;
        World *_world;
        bool *_serverIsRunning;
        std::queue<std::string> _broadcastQueue;

        void clientEventHandling();
        void broadcastGuiInfo();
        void broadcastMessageToClients();

        /// @brief Factory method: creates the CommandParser for a newly accepted client.
        /// Subclasses override this to inject a specialised parser.
        virtual std::unique_ptr<CommandParser> makeParser(Client *client, World *world, std::queue<std::string> *broadcastQueue);

    public:
        ClientHandler(int port, int initialClientCapacity, World *world = nullptr, bool *serverIsRunning = nullptr);
        virtual ~ClientHandler();

        virtual void handleClients(void);
        virtual void addClient();

        void removeClient(int fd, const std::string &reason = "unknown");
        Client *getClientByFd(int fd) const;
        Client *getClientByPlayerId(int playerId) const;
        std::queue<std::string> &getBroadcastQueue();
    };
} // namespace zappy
