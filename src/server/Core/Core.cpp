#include "Core.hpp"

#define MAX_MAP_SIZE 1024
#define MAX_CLIENTS 256
#define MAX_TEAMS 4
#define MAX_F 1000

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
    if (_argParser.getInt("-p") > 65535)
        throw ServerException("Port number must be between 0 and 65535");
    if (_argParser.getInt("-x") < 1 || _argParser.getInt("-y") < 1 || _argParser.getInt("-x") > MAX_MAP_SIZE || _argParser.getInt("-y") > MAX_MAP_SIZE)
        throw ServerException("World dimensions must be positive integers and countains between 1 and " + std::to_string(MAX_MAP_SIZE));
    if (_argParser.getInt("-c") > MAX_CLIENTS)
        throw ServerException("Number of initial clients per team must be between 0 and " + std::to_string(MAX_CLIENTS));
    if (_argParser.getList("-n").size() < 1 || _argParser.getList("-n").size() > MAX_TEAMS)
        throw ServerException("Number of teams must be between 1 and " + std::to_string(MAX_TEAMS));
    if (_argParser.hasFlag("-f"))
        if (_argParser.getInt("-f") < 1 || _argParser.getInt("-f") > MAX_F)
            throw ServerException("Invalid value for flag '-f', must be between 1 and " + std::to_string(MAX_F));
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