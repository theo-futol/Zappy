#pragma once
#include <string>
#include <queue>
#include <vector>
#include <functional>
#include <unordered_map>
#include <chrono>
#include "../../Client/Client.hpp"

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
        using Handler = std::function<void(const std::vector<std::string> &)>;

        CommandParser(Client *client, int f);

        /// @brief Reads pending data from the client socket into the command queue.
        void feed();

        /// @brief Dispatches the next queued command if its time has come.
        void executeNext();

        /// @brief Returns true if there are commands waiting in the queue.
        bool hasPending() const;

    private:

        Client *_client;
        int _f;
        std::queue<PendingCommand> _commandQueue;
        std::unordered_map<std::string, std::pair<int, Handler>> _aiCommands;
        std::unordered_map<std::string, Handler> _graphicCommands;
        void _initAICommands();
        void _initGraphicCommands();
        void _dispatch(const std::string &line);
        void _handleHandshake(const std::string &teamName);
    };
} // namespace zappy
