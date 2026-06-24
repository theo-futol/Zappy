#include "Network/service/NetworkService.hpp"
#include "Command/commandbuilder/CommandBuilder.hpp"
#include "Protocol/handler/egg/EggEventHandler.hpp"
#include "Protocol/handler/entitycommon/EntityCommonHandler.hpp"
#include "Protocol/handler/game/GameEventHandler.hpp"
#include "Protocol/handler/incantation/IncantationEventHandler.hpp"
#include "Protocol/handler/map/MapEventHandler.hpp"
#include "Protocol/handler/team/TeamEventHandler.hpp"
#include "Protocol/handler/trantorian/TrantorianEventHandler.hpp"

namespace Zappy
{

NetworkService::NetworkService(GameState &state, std::unique_ptr<INetwork> socket) : _state(state), _registry(), _socket(std::move(socket)), _parser(_registry, _state)
{
    registerHandlers();
}

void NetworkService::registerHandlers()
{
    _registry.registerHandler(std::make_unique<MapEventHandler>());
    _registry.registerHandler(std::make_unique<TrantorianEventHandler>());
    _registry.registerHandler(std::make_unique<EggEventHandler>());
    _registry.registerHandler(std::make_unique<EntityCommonHandler>());
    _registry.registerHandler(std::make_unique<TeamEventHandler>());
    _registry.registerHandler(std::make_unique<IncantationEventHandler>());
    _registry.registerHandler(std::make_unique<GameEventHandler>());
}

void NetworkService::connect(const std::string &host, int port)
{
    // The socket lives behind INetwork; catch its exception contract at that seam and
    // rethrow our own (hierarchical propagation).
    try
    {
        _socket->connect(host, port);
        _socket->send("GRAPHIC\n");
        _socket->send(CommandBuilder::requestMapSize());
        _socket->send(CommandBuilder::requestMapContent());
        _socket->send(CommandBuilder::requestTeamNames());
        _socket->send(CommandBuilder::requestTimeUnit());
    }
    catch (const INetwork::INetworkException &e)
    {
        throw NetworkServiceException(std::string("connect: ") + e.what());
    }
}

void NetworkService::update()
{
    try
    {
        std::string data = _socket->receive();

        while (!data.empty())
        {
            _parser.feed(data);
            data = _socket->receive();
        }
    }
    catch (const INetwork::INetworkException &e)
    {
        throw NetworkServiceException(std::string("update: ") + e.what());
    }
}

} // namespace Zappy
