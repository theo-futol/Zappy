#include "Core/Core.hpp"
#include "Network/service/NetworkService.hpp"
#include "Network/socket/TcpSocket.hpp"
#include "interface/INetwork.hpp"
#include <stdexcept>
#include <unistd.h>

namespace Zappy
{

Core::Core(int argc, char **argv) : _state(), _network(nullptr), _render(nullptr), _host("localhost"), _port(0)
{
    parseArguments(argc, argv);
}

void Core::parseArguments(int argc, char **argv)
{
    bool hasPort = false;
    bool hasHost = false;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "-p" && i + 1 < argc)
        {
            _port = parsePort(argv[++i]);
            hasPort = true;
        }
        else if (arg == "-h" && i + 1 < argc)
        {
            _host = argv[++i];
            hasHost = true;
        }
        else
        {
            throw CoreException(USAGE);
        }
    }
    if (!hasPort || !hasHost)
        throw CoreException(USAGE);
}

int Core::parsePort(const std::string &value)
{
    int port = 0;

    try
    {
        port = std::stoi(value);
    }
    catch (const std::invalid_argument &)
    {
        throw CoreException("Invalid port: " + value);
    }
    catch (const std::out_of_range &)
    {
        throw CoreException("Port out of range: " + value);
    }
    if (port <= 0 || port > 65535)
        throw CoreException("Invalid port: " + value);
    return port;
}

void Core::init()
{
    try
    {
        _network = std::make_unique<NetworkService>(_state, std::make_unique<TcpSocket>());
        _network->connect(_host, _port);
        _render = std::make_unique<RenderSystem>(1280, 720, "Zappy");
        _render->init();
    }
    catch (const NetworkService::NetworkServiceException &e)
    {
        throw CoreException("Network service error: " + std::string(e.what()));
    }
    catch (const INetwork::INetworkException &e)
    {
        throw CoreException("Network socket error: " + std::string(e.what()));
    }
    catch (const RenderSystem::RenderSystemException &e)
    {
        throw CoreException("Render system error: " + std::string(e.what()));
    }
}

void Core::run()
{
    while (_render->processInput(_state))
    {
        try
        {
            for (const std::string &command : _render->takeOutgoing())
                _network->send(command);
            _network->update();
        }
        catch (const NetworkService::NetworkServiceException &e)
        {
            throw CoreException("Network error: " + std::string(e.what()));
        }
        _render->render(_state);
    }
}

} // namespace Zappy
