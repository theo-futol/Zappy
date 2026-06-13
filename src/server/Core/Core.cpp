#include "Core.hpp"

namespace zappy
{
Core::Core(ArgParser argParser) : _argParser(std::move(argParser)), _clientHandler(nullptr), _world(nullptr)
{
    _argParser.registerFlag("-p", zappy::FlagType::INT, true, "port number");
    _argParser.registerFlag("-x", zappy::FlagType::INT, true, "width of the world");
    _argParser.registerFlag("-y", zappy::FlagType::INT, true, "height of the world");
    _argParser.registerFlag("-n", zappy::FlagType::LIST, true, "name of the team");
    _argParser.registerFlag("-c", zappy::FlagType::INT, true, "number of initial clients per team");
    _argParser.registerFlag("-f", zappy::FlagType::INT, false, "reciprocal of time unit for execution of actions");
    _argParser.parse();
}

void Core::setClientHandler()
{
    int port = _argParser.getInt("-p");
    int f = _argParser.hasFlag("-f") ? _argParser.getInt("-f") : 100;
    int initialClientCapacity = _argParser.getInt("-c") * _argParser.getList("-n").size();
    _clientHandler = std::make_unique<ClientHandler>(port, initialClientCapacity, f);
}

void Core::setWorld()
{
    int width = _argParser.getInt("-x");
    int height = _argParser.getInt("-y");
    _world = std::make_unique<World>(width, height);
}

void Core::run()
{
    _clientHandler->handleClients();
}
} // namespace zappy