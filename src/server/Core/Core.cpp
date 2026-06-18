#include "Core.hpp"

namespace zappy
{

bool Core::serverIsRunning = true;

Core::Core(ArgParser argParser) : _argParser(std::move(argParser)), _clientHandler(nullptr), _world(nullptr)
{
    setSignalHandler();
    _argParser.registerFlag("-p", zappy::FlagType::INT, true, "port number");
    _argParser.registerFlag("-x", zappy::FlagType::INT, true, "width of the world");
    _argParser.registerFlag("-y", zappy::FlagType::INT, true, "height of the world");
    _argParser.registerFlag("-n", zappy::FlagType::LIST, true, "name of the team");
    _argParser.registerFlag("-c", zappy::FlagType::INT, true, "number of initial clients per team");
    _argParser.registerFlag("-f", zappy::FlagType::INT, false, "reciprocal of time unit for execution of actions");
    _argParser.parse();
    serverIsRunning = true;
}

void Core::setClientHandler()
{
    int port = _argParser.getInt("-p");
    int f = _argParser.hasFlag("-f") ? _argParser.getInt("-f") : 100;
    int initialClientCapacity = _argParser.getInt("-c") * _argParser.getList("-n").size();
    _clientHandler = std::make_unique<ClientHandler>(port, initialClientCapacity, f, _world.get(), &serverIsRunning);
    setWorldBroadcast();
}

void Core::setWorldBroadcast()
{
    if (_world)
        _world->setBroadCastQueue(&_clientHandler->getBroadcastQueue());
}

void Core::setWorld()
{
    int width = _argParser.getInt("-x");
    int height = _argParser.getInt("-y");
    int initialSlots = _argParser.getInt("-c");
    _world = std::make_unique<World>(width, height);
    const auto &teamNames = _argParser.getList("-n");
    for (int i = 0; i < static_cast<int>(teamNames.size()); ++i)
        _world->addTeam(teamNames[i], i, initialSlots);
    std::cout << "World created with dimensions: " << width << "x" << height << std::endl;
}

void Core::signalHandler(int signal)
{
    if (signal == SIGINT || signal == SIGTERM)
        serverIsRunning = false;
}

void Core::setSignalHandler()
{
    std::signal(SIGINT, &Core::signalHandler);
    std::signal(SIGTERM, &Core::signalHandler);
}

void Core::run()
{
    _clientHandler->handleClients();
}
} // namespace zappy