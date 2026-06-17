#pragma once
#include <string>
#include <queue>
#include <vector>
#include <functional>
#include <unordered_map>
#include <chrono>

#include "../../Client/Client.hpp"
#include "../../../Simulation/World/World.hpp"
#include "../Commands/Commands.hpp"

namespace zappy
{
    struct PendingCommand
    {
        std::string line;
        std::chrono::steady_clock::time_point readyAt;
    };

    class CommandParser
    {
    public:
        using Handler = std::function<void(const std::vector<std::string> &cmd, Client &client, Commands &commands)>;

        CommandParser(Client *client, int f, World *world = nullptr, std::queue<std::string> *broadcastQueue = nullptr);

        /// @brief Reads pending data from the client socket into the command queue.
        /// @return false if the client disconnected.
        bool feed();

        /// @brief Dispatches the next queued command if its time has come.
        void executeNext();

        /// @brief Returns true if there are commands waiting in the queue.
        bool hasPending() const;

        /// @brief Returns the readyAt time of the next queued command, or time_point::max() if the queue is empty.
        std::chrono::steady_clock::time_point nextReadyAt() const;

    private:

        Client *_client;
        int _f;
        World *_world;
        std::queue<PendingCommand> _commandQueue;
        std::unordered_map<std::string, std::pair<int, Handler>> _aiCommands;
        std::unordered_map<std::string, Handler> _graphicCommands;
        Commands _commands;
        std::queue<std::string> *_broadcastQueue;
        void _initAICommands();
        void _initGraphicCommands();
        void _dispatch(const std::string &line);
        void _handleHandshake(const std::string &teamName);
    };
} // namespace zappy
