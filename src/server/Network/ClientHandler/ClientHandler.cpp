#include "ClientHandler.hpp"
#include "../../Logger/Logger.hpp"
#include "../../ServerException/ServerException.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <sys/socket.h>

static constexpr int RESOURCE_INTERVAL_TIME_UNITS = 20;
static constexpr int CYCLE_TO_DIE = 126;

namespace zappy
{
ClientHandler::~ClientHandler()
{
}

ClientHandler::ClientHandler(int port, int initialClientCapacity, World *world, bool *serverIsRunning)
    : _lastResourceUpdate(std::chrono::steady_clock::now()), _lastFoodUpdate(std::chrono::steady_clock::now()), _world(world), _serverIsRunning(serverIsRunning), _broadcastQueue()
{
    _tcpSocket.create(AF_INET, SOCK_STREAM, 0);
    _tcpSocket.bind(port);
    _tcpSocket.listen();

    _fds.reserve(initialClientCapacity + 1);
    _fds.push_back({.fd = _tcpSocket.getFd(), .events = POLLIN, .revents = 0});
}

void ClientHandler::handleClients(void)
{
    Logger::log("000", "Poll : initialized successfully");
    while (*_serverIsRunning)
    {
        auto now = std::chrono::steady_clock::now();
        auto foodIntervalMs = std::chrono::milliseconds(CYCLE_TO_DIE * 1000 / _world->getTimeUnit());
        auto resourceIntervalMs = std::chrono::milliseconds(RESOURCE_INTERVAL_TIME_UNITS * 1000 / _world->getTimeUnit());
        auto deadline = _lastResourceUpdate + resourceIntervalMs;
        auto foodDeadline = _lastFoodUpdate + foodIntervalMs;
        if (foodDeadline < deadline)
            deadline = foodDeadline;
        for (auto &[fd, parser] : _parsers)
        {
            auto t = parser->nextReadyAt();
            if (t < deadline)
                deadline = t;
        }
        int64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();
        int timeout = std::max(static_cast<int64_t>(0), ms);

        if (poll(_fds.data(), _fds.size(), timeout) < 0)
        {
            if (errno == EINTR)
                continue;
            if (errno == ENOMEM)
                Logger::log("8401", "Poll : failed, out of memory (ENOMEM)");
            else
                Logger::log("8400", "Poll : failed, unknown error", {{"errno", std::to_string(errno)}});
            throw ServerException("Poll failed: " + std::string(strerror(errno)));
        }
        if (_fds[0].revents & POLLIN)
            addClient();

        now = std::chrono::steady_clock::now();
        if (now - _lastResourceUpdate >= resourceIntervalMs)
        {
            if (!_world->isPaused())
                _world->resourcePassiveGeneration();
            _lastResourceUpdate = now;
        }
        // if (now - _lastFoodUpdate >= foodIntervalMs)
        // {
        //     if (!_world->isPaused())
        //     {
        //         for (int id : _world->foodCheck())
        //         {
        //             Client *client = getClientByPlayerId(id);
        //             if (client)
        //                 client->setType(ClientType::DEAD);
        //         }
        //     }
        //     _lastFoodUpdate = now;
        // }
        clientEventHandling();
        if (_world->checkWinningCondition())
            *_serverIsRunning = false;
        broadcastGuiInfo();
        broadcastMessageToClients();
        Team *winningTeam = _world->getWinningTeam();
        if (winningTeam)
            for (const auto &player : _world->getPlayers())
                if (player->getTeam()._name == winningTeam->_name)
                    std::cout << "Player " << player->getId() << " was level: " << player->getLevel() << std::endl;
    }
}

void ClientHandler::broadcastMessageToClients()
{
    std::clock_t currentTime = std::clock();

    for (const auto &client : _clients)
        if (client->getType() == ClientType::AI)
        {
            Player *player = _world->getPlayerById(client->getPlayerId());
            if (player)
                player->sendMessageToClient(currentTime, _clients);
        }
}

void ClientHandler::clientEventHandling()
{
    for (size_t i = 1; i < _fds.size(); i++)
    {
        if (_fds[i].revents & POLLHUP)
        {
            Client *client = getClientByFd(_fds[i].fd);
            Player *player = client ? _world->getPlayerById(client->getPlayerId()) : nullptr;
            Logger::log("121", "Client : disconnection detected", {{"fd", std::to_string(_fds[i].fd)}, {"player_id", player ? std::to_string(player->getId()) : ""}});
            removeClient(_fds[i].fd, "peer closed connection");
            i--;
            continue;
        }
        if (_fds[i].revents & POLLIN)
        {
            auto it = _parsers.find(_fds[i].fd);
            if (it != _parsers.end())
            {
                if (!it->second->feed(_clients))
                {
                    Client *client = getClientByFd(_fds[i].fd);
                    Player *player = client ? _world->getPlayerById(client->getPlayerId()) : nullptr;
                    Logger::log("121", "Client : disconnection detected", {{"fd", std::to_string(_fds[i].fd)}, {"player_id", player ? std::to_string(player->getId()) : ""}});
                    removeClient(_fds[i].fd, "recv failed or client closed connection");
                    i--;
                    continue;
                }
            }
        }
    }
    std::vector<int> fdsToRemove;
    for (auto &[fd, parser] : _parsers)
    {
        if (parser->isBanned())
            fdsToRemove.push_back(fd);
        else
        {
            while (parser->executeNext(_clients))
                ;
        }
    }
    for (int fd : fdsToRemove)
    {
        Client *client = getClientByFd(fd);
        Player *player = client ? _world->getPlayerById(client->getPlayerId()) : nullptr;
        Logger::log("121", "Client : disconnection detected", {{"fd", std::to_string(fd)}, {"player_id", player ? std::to_string(player->getId()) : ""}});
        removeClient(fd, "command queue overflow");
    }
}

void ClientHandler::addClient()
{
    int clientFd = _tcpSocket.accept();
    if (clientFd < 0)
        return;
    Logger::log("120", "Client : new connection attempt", {{"fd", std::to_string(clientFd)}});
    send(clientFd, "WELCOME\n", 8, MSG_NOSIGNAL);
    Logger::log("030", "Client : WELCOME handshake sent", {{"fd", std::to_string(clientFd)}});
    _clients.push_back(std::make_unique<Client>(clientFd));
    Logger::log("030", "Client : connection established", {{"fd", std::to_string(clientFd)}, {"type", "UNKNOWN"}});
    _parsers[clientFd] = std::make_unique<CommandParser>(_clients.back().get(), _world, &_broadcastQueue);
    Logger::log("030", "Client : command parser created", {{"fd", std::to_string(clientFd)}});
    _fds.push_back({.fd = clientFd, .events = POLLIN, .revents = 0});
    Logger::log("030", "Client : connection established", {{"fd", std::to_string(clientFd)}, {"type", "UNKNOWN"}});
}

void ClientHandler::removeClient(int fd, const std::string &reason)
{
    Client *client = getClientByFd(fd);
    int playerIdValue = client ? client->getPlayerId() : -1;
    std::string playerId = playerIdValue >= 0 ? std::to_string(playerIdValue) : "";
    if (reason == "peer closed connection")
        Logger::log("032", "Client : disconnected cleanly", {{"fd", std::to_string(fd)}, {"player_id", playerId}});
    else
        Logger::log("8432", "Client : disconnected unexpectedly", {{"fd", std::to_string(fd)}, {"player_id", playerId}});

    if (playerIdValue >= 0)
        _world->removePlayer(playerIdValue);
    _parsers.erase(fd);

    _clients.erase(std::remove_if(_clients.begin(), _clients.end(), [fd](const std::unique_ptr<Client> &client) { return client->getFd() == fd; }), _clients.end());

    _fds.erase(std::remove_if(_fds.begin(), _fds.end(), [fd](const pollfd &pfd) { return pfd.fd == fd; }), _fds.end());
}

Client *ClientHandler::getClientByFd(int fd) const
{
    for (const auto &client : _clients)
        if (client->getFd() == fd)
            return client.get();
    return nullptr;
}

Client *ClientHandler::getClientByPlayerId(int playerId) const
{
    for (const auto &client : _clients)
        if (client->getPlayerId() == playerId)
            return client.get();
    return nullptr;
}

void ClientHandler::broadcastGuiInfo()
{
    while (!_broadcastQueue.empty())
    {
        std::string message = _broadcastQueue.front();
        _broadcastQueue.pop();
        for (const auto &client : _clients)
            if (client->getType() == ClientType::GRAPHIC)
                send(client->getFd(), message.c_str(), message.size(), MSG_NOSIGNAL);
    }
}

std::queue<std::string> &ClientHandler::getBroadcastQueue()
{
    return _broadcastQueue;
}

} // namespace zappy
