#include "Core.hpp"
#include "../Network/ClientHandler/WaitClientHandler/WaitClientHandler.hpp"
#include "../ServerException/ServerException.hpp"
#include <algorithm>
#include <iostream>

#define MAX_MAP_SIZE 1024
#define MAX_CLIENTS 256
#define MAX_TEAMS 4
#define MAX_F 1000

#define INITIAL_CLIENT_CAPACITY 24
#define INITIAL_WORLD_WIDTH 25
#define INITIAL_WORLD_HEIGHT 25

namespace zappy
{

bool Core::serverIsRunning = true;

Core::Core(ArgParser argParser) : _argParser(std::move(argParser)), _clientHandler(nullptr), _world(nullptr)
{
    setSignalHandler();
    _argParser.registerFlag("-p", zappy::FlagType::INT, false, "port number");
    _argParser.registerFlag("-x", zappy::FlagType::INT, false, "width of the world");
    _argParser.registerFlag("-y", zappy::FlagType::INT, false, "height of the world");
    _argParser.registerFlag("-n", zappy::FlagType::LIST, false, "name of the team");
    _argParser.registerFlag("-c", zappy::FlagType::INT, false, "number of initial clients per team");
    _argParser.registerFlag("-f", zappy::FlagType::INT, false, "reciprocal of time unit for execution of actions");
    _argParser.registerFlag("-oldgen", zappy::FlagType::FLAG, false, "use the legacy resource generation algorithm");
    _argParser.registerFlag("--wait-timeout", zappy::FlagType::INT, false, "per-turn wait timeout in milliseconds; enables serial-turn mode");
    _argParser.parse();
    if (_argParser.hasFlag("-p"))
        if (_argParser.getInt("-p") > 65535)
            throw ServerException("Port number must be between 0 and 65535");
    if (_argParser.hasFlag("-x"))
        if (_argParser.getInt("-x") < 1 || _argParser.getInt("-x") > MAX_MAP_SIZE)
            throw ServerException("World dimensions must be positive integers and countains between 1 and " + std::to_string(MAX_MAP_SIZE));
    if (_argParser.hasFlag("-y"))
        if (_argParser.getInt("-y") < 1 || _argParser.getInt("-y") > MAX_MAP_SIZE)
            throw ServerException("World dimensions must be positive integers and countains between 1 and " + std::to_string(MAX_MAP_SIZE));
    if (_argParser.hasFlag("-c"))
        if (_argParser.getInt("-c") > MAX_CLIENTS)
            throw ServerException("Number of initial clients per team must be between 0 and " + std::to_string(MAX_CLIENTS));
    if (_argParser.hasFlag("-n"))
    {
        std::vector<std::string> teamNames = _argParser.getList("-n");
        if (teamNames.size() < 1 || teamNames.size() > MAX_TEAMS || std::any_of(teamNames.begin(), teamNames.end(), [](const std::string &name) { return name.empty(); }))
            throw ServerException("Number of teams must be between 1 and " + std::to_string(MAX_TEAMS));
        for (const auto &teamName : teamNames)
        {
            if (std::count(teamNames.begin(), teamNames.end(), teamName) > 1)
                throw ServerException("Team names must be unique");
        }
    }

    if (_argParser.hasFlag("-f"))
        if (_argParser.getInt("-f") < 1 || _argParser.getInt("-f") > MAX_F)
            throw ServerException("Invalid value for flag '-f', must be between 1 and " + std::to_string(MAX_F));
    if (_argParser.hasFlag("--wait-timeout"))
        if (_argParser.getInt("--wait-timeout") < 1)
            throw ServerException("--wait-timeout must be a positive integer (milliseconds)");
    serverIsRunning = true;
}

void Core::setClientHandler()
{
    int port = _argParser.hasFlag("-p") ? _argParser.getInt("-p") : 4242;
    int cFlag = _argParser.hasFlag("-c") ? _argParser.getInt("-c") : INITIAL_CLIENT_CAPACITY;
    int nFlag = _argParser.hasFlag("-n") ? static_cast<int>(_argParser.getList("-n").size()) : 10;
    int initialClientCapacity = cFlag * nFlag;
    if (_argParser.hasFlag("--wait-timeout"))
    {
        int waitTimeout = _argParser.getInt("--wait-timeout");
        _clientHandler = std::make_unique<WaitClientHandler>(port, initialClientCapacity, _world.get(), &serverIsRunning, waitTimeout);
    }
    else
    {
        _clientHandler = std::make_unique<ClientHandler>(port, initialClientCapacity, _world.get(), &serverIsRunning);
    }
    setWorldBroadcast();
}

void Core::setWorldBroadcast()
{
    if (_world)
        _world->setBroadCastQueue(&_clientHandler->getBroadcastQueue());
}

void Core::setWorld()
{
    int width = _argParser.hasFlag("-x") ? _argParser.getInt("-x") : INITIAL_WORLD_WIDTH;
    int height = _argParser.hasFlag("-y") ? _argParser.getInt("-y") : INITIAL_WORLD_HEIGHT;
    int f = _argParser.hasFlag("-f") ? _argParser.getInt("-f") : 100;
    int initialSlots = _argParser.hasFlag("-c") ? _argParser.getInt("-c") : INITIAL_CLIENT_CAPACITY;
    _world = std::make_unique<World>(width, height, f, _argParser.hasFlag("-oldgen"));
    const auto &teamNames = _argParser.hasFlag("-n") ? _argParser.getList("-n") : std::vector<std::string>{"Team1", "Team2"};
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