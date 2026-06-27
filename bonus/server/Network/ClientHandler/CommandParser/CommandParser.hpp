#pragma once
#include <string>
#include <deque>
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
    /// @brief A command waiting in a client's queue together with the time it may run.
    struct PendingCommand
    {
        std::string line;
        std::chrono::steady_clock::time_point readyAt;
    };

    /// @brief Per-client command pipeline: reads socket data, splits it into commands,
    ///        and runs them at the right time according to their in-game duration.
    class CommandParser
    {
    public:
        using Handler = std::function<std::string(const std::vector<std::string> &cmd, Client &client, Commands &commands, std::vector<std::unique_ptr<Client>> &clients)>;

        CommandParser(Client *client, World *world = nullptr, std::queue<std::string> *broadcastQueue = nullptr);
        virtual ~CommandParser() = default;

        virtual bool feed(std::vector<std::unique_ptr<Client>> &clients);
        virtual bool executeNext(std::vector<std::unique_ptr<Client>> &clients);
        bool isBanned() const;
        virtual std::chrono::steady_clock::time_point nextReadyAt() const;

    protected:
        Client *_client;
        World *_world;
        std::deque<PendingCommand> _commandQueue;
        std::unordered_map<std::string, std::pair<int, Handler>> _aiCommands;
        std::unordered_map<std::string, Handler> _graphicCommands;
        Commands _commands;
        std::queue<std::string> *_broadcastQueue;
        bool _isBanned;

        /// @brief Returns the time base used for the next command's readyAt.
        /// Subclasses may override to use a different base (e.g. last-turn-ended time).
        virtual std::chrono::steady_clock::time_point commandQueueBase() const;

    private:
        void _initAICommands();
        void _initGraphicCommands();
        void _dispatch(const std::string &line, std::vector<std::unique_ptr<Client>> &clients);
        void _handleHandshake(const std::string &teamName);
    };
} // namespace zappy
