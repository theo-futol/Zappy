#pragma once
#include "../ArgParser/ArgParser.hpp"
#include "../Network/ClientHandler/ClientHandler.hpp"

namespace zappy
{
    class Core
    {
        public:
            Core(ArgParser argParser);
            ~Core() = default;

            void setClientHandler();
            void setWorld();
            void run();
        private:
            ArgParser _argParser;
            std::unique_ptr<ClientHandler> _clientHandler;
            std::unique_ptr<World> _world;
    };
} // namespace zappy
