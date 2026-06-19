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
    /// @brief A command waiting in a client's queue together with the time it may run.
    ///
    /// Many commands take in-game time to complete, so they are not executed on arrival
    /// but when steady_clock reaches readyAt.
    struct PendingCommand
    {
        std::string line;                              ///< Raw command line as received.
        std::chrono::steady_clock::time_point readyAt; ///< Earliest time the command may execute.
    };

    /// @brief Per-client command pipeline: reads socket data, splits it into commands,
    ///        and runs them at the right time according to their in-game duration.
    ///
    /// Each client gets its own CommandParser. It holds the command tables (one for AI
    /// clients, one for graphic clients) and turns each parsed line into a call on the
    /// shared Commands object.
    class CommandParser
    {
    public:
        /// @brief Signature every command handler implements.
        using Handler = std::function<void(const std::vector<std::string> &cmd, Client &client, Commands &commands)>;

        /// @brief Builds a parser for one client.
        /// @param client         The client this parser serves.
        /// @param f              Reciprocal of the time unit, used to scale command durations.
        /// @param world          The shared simulation world.
        /// @param broadcastQueue Queue for messages bound for graphic clients.
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

        Client *_client;                       ///< Client this parser serves (not owned).
        int _f;                                ///< Reciprocal of the time unit, scales durations.
        World *_world;                         ///< Shared simulation world (not owned).
        std::queue<PendingCommand> _commandQueue; ///< Commands awaiting their readyAt time.
        std::unordered_map<std::string, std::pair<int, Handler>> _aiCommands;   ///< AI command name -> (duration, handler).
        std::unordered_map<std::string, Handler> _graphicCommands;             ///< Graphic command name -> handler.
        Commands _commands;                    ///< Shared implementation of the command behaviours.
        std::queue<std::string> *_broadcastQueue; ///< Queue for messages to graphic clients (not owned).

        /// @brief Fills _aiCommands with the AI protocol commands and their durations.
        void _initAICommands();
        /// @brief Fills _graphicCommands with the graphic protocol commands.
        void _initGraphicCommands();
        /// @brief Routes one command line to the matching handler for the client's type.
        void _dispatch(const std::string &line);
        /// @brief Handles the first line of an AI client: the team-name handshake.
        void _handleHandshake(const std::string &teamName);
    };
} // namespace zappy
