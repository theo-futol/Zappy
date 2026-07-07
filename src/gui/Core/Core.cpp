#include "Core/Core.hpp"
#include "Network/service/NetworkService.hpp"
#include "Network/socket/TcpSocket.hpp"
#include "interface/INetwork.hpp"
#include <stdexcept>
#include <unistd.h>

namespace Zappy
{

Core::Core(int argc, char **argv)
    : _state(), _window(nullptr), _context(nullptr), _menu(nullptr), _network(nullptr), _render(nullptr), _host(), _port(0), _mode(RenderMode::TwoD), _vrRequested(false)
{
    parseArguments(argc, argv);
}

void Core::parseArguments(int argc, char **argv)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "-p" && i + 1 < argc)
            _port = parsePort(argv[++i]);
        else if (arg == "-h" && i + 1 < argc)
            _host = argv[++i];
        else if (arg == "-m" && i + 1 < argc)
            _mode = parseMode(argv[++i]);
        else if (arg == "--vr")
            _vrRequested = true;
        else
            throw CoreException(USAGE);
    }
}

RenderMode Core::parseMode(const std::string &value)
{
    if (value == "2d")
        return RenderMode::TwoD;
    if (value == "3d")
        return RenderMode::ThreeD;
    if (value == "torus" || value == "3dtorus")
        return RenderMode::ThreeDTorus;
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
        _window = std::make_unique<Window>(WindowWidth, WindowHeight, WindowTitle);
        _context = std::make_unique<GraphicsContext>();
        _context->configureDefaults();
        _menu = std::make_unique<MainMenu>(*_window, *_context);
        _menu->init();
    }
    catch (const Window::WindowException &e)
    {
        throw CoreException("Window error: " + std::string(e.what()));
    }
    catch (const MainMenu::MainMenuException &e)
    {
        throw CoreException("Menu error: " + std::string(e.what()));
    }
}

bool Core::runSession(std::string &error)
{
    bool backToMenu = false;

    _state = GameState();
    try
    {
        _network = std::make_unique<NetworkService>(_state, std::make_unique<TcpSocket>());
        _network->connect(_host, _port);
        _render = std::make_unique<RenderSystem>(*_window, *_context, _mode, _vrRequested);
        _render->init();
        if (_vrRequested && !_render->vrEnabled())
            error = _render->vrWarning();
    }
    catch (const NetworkService::NetworkServiceException &e)
    {
        error = std::string("Connexion impossible: ") + e.what();
        _render.reset();
        _network.reset();
        return true;
    }
    catch (const INetwork::INetworkException &e)
    {
        error = std::string("Connexion impossible: ") + e.what();
        _render.reset();
        _network.reset();
        return true;
    }
    catch (const RenderSystem::RenderSystemException &e)
    {
        throw CoreException("Render system error: " + std::string(e.what()));
    }
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
            error = std::string("Connexion perdue: ") + e.what();
            backToMenu = true;
            break;
        }
        _render->render(_state);
    }
    if (_render->wantsMenu())
        backToMenu = true;
    _render.reset();
    _network.reset();
    return backToMenu;
}

void Core::run()
{
    std::string error;

    while (true)
    {
        _menu->setDefaults(_host, _port, _mode, _vrRequested);
        _menu->setError(error);
        error.clear();

        MenuConfig config = _menu->run();

        if (config.result == MenuResult::Quit)
            break;
        _host = config.host;
        _port = config.port;
        _mode = config.mode;
        _vrRequested = config.vr;
        if (!runSession(error))
            break;
    }
}

} // namespace Zappy
