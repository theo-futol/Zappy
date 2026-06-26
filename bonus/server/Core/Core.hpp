#pragma once

#include <csignal>

#include "../ArgParser/ArgParser.hpp"
#include "../Network/ClientHandler/ClientHandler.hpp"

namespace zappy
{
    /// @brief Top-level server object: owns the world and the network handler and
    ///        wires them together from the parsed command-line arguments.
    ///
    /// Typical lifecycle: construct with an ArgParser, then call run(), which builds
    /// the world and client handler and enters the blocking network loop until a
    /// signal flips serverIsRunning to false.
    class Core
    {
        public:
            /// @brief Stores the parsed arguments; does not build the world or network yet.
            Core(ArgParser argParser);
            ~Core() = default;

            /// @brief Creates the ClientHandler from the args (port, client count, time unit).
            void setClientHandler();

            /// @brief Creates the World from the args (map size, teams).
            void setWorld();

            /// @brief Installs the process signal handler used for graceful shutdown.
            void setSignalHandler();

            /// @brief Signal handler that clears serverIsRunning to stop the main loop.
            static void signalHandler(int signal);

            /// @brief Builds the world and network handler, then runs the blocking event loop.
            void run();

            /// @brief Global run flag; set to false by signalHandler() to end the loop.
            static bool serverIsRunning;
        private:
            ArgParser _argParser;
            std::unique_ptr<ClientHandler> _clientHandler;
            std::unique_ptr<World> _world;
            std::string _rulesContent;

            /// @brief Connects the world's broadcast queue to the client handler so
            ///        world events can be pushed out to clients.
            void setWorldBroadcast();
    };
} // namespace zappy
