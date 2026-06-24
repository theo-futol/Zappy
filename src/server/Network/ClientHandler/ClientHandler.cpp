#include "ClientHandler.hpp"
#include <iostream>
#include <sys/socket.h>

static constexpr int RESOURCE_INTERVAL_TIME_UNITS = 20;
static constexpr int CYCLE_TO_DIE = 126;

namespace zappy
{
ClientHandler::~ClientHandler()
{
}

ClientHandler::ClientHandler(int port, int initialClientCapacity, int f, World *world, bool *serverIsRunning)
    : _f(f), _lastResourceUpdate(std::chrono::steady_clock::now()), _lastFoodUpdate(std::chrono::steady_clock::now()), _world(world), _serverIsRunning(serverIsRunning),
      _broadcastQueue()
{
    _tcpSocket.create(AF_INET, SOCK_STREAM, 0);
    _tcpSocket.bind(port);
    _tcpSocket.listen();

    _fds.reserve(initialClientCapacity + 1);
    _fds.push_back({.fd = _tcpSocket.getFd(), .events = POLLIN, .revents = 0});
}

void ClientHandler::handleClients(void)
{
    while (*_serverIsRunning)
    {
        auto now = std::chrono::steady_clock::now();
        auto foodIntervalMs = std::chrono::milliseconds(CYCLE_TO_DIE * 1000 / _f);
        auto resourceIntervalMs = std::chrono::milliseconds(RESOURCE_INTERVAL_TIME_UNITS * 1000 / _f);
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
            throw ServerException("Poll failed: " + std::string(strerror(errno)));
        }
        if (_fds[0].revents & POLLIN)
            addClient();

        now = std::chrono::steady_clock::now();
        if (now - _lastResourceUpdate >= resourceIntervalMs)
        {
            _world->ressourcePassiveGeneration();
            _lastResourceUpdate = now;
        }
        if (now - _lastFoodUpdate >= foodIntervalMs)
        {
            for (int fd : _world->foodCheck())
            {
                Client *client = getClientByFd(fd);
                if (client)
                    client->setType(ClientType::DEAD);
            }
            _lastFoodUpdate = now;
        }
        clientEventHandling();
        if (_world->checkWinningCondition())
            *_serverIsRunning = false;
        broadcastGuiInfo();
        broadcastMessageToClients();
    }
}

void ClientHandler::broadcastMessageToClients()
{
    for (const auto &client : _clients)
        if (client->getType() == ClientType::AI)
        {
            Player *player = _world->getPlayerByFd(client->getFd());
            if (player)
                player->sendMessageToClient();
        }
}

void ClientHandler::clientEventHandling()
{
    for (size_t i = 1; i < _fds.size(); i++)
    {
        if (_fds[i].revents & POLLHUP)
        {
            removeClient(_fds[i].fd, "peer closed connection");
            i--;
            continue;
        }
        if (_fds[i].revents & POLLIN)
        {
            auto it = _parsers.find(_fds[i].fd);
            if (it != _parsers.end())
            {
                if (!it->second->feed())
                {
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
            while (parser->executeNext())
                ;
        }
    }
    for (int fd : fdsToRemove)
        removeClient(fd, "command queue overflow");
}

void ClientHandler::addClient()
{
    int clientFd = _tcpSocket.accept();
    if (clientFd < 0)
        return;
    send(clientFd, "WELCOME\n", 8, MSG_NOSIGNAL);
    _clients.push_back(std::make_unique<Client>(clientFd));
    _parsers[clientFd] = std::make_unique<CommandParser>(_clients.back().get(), _f, _world, &_broadcastQueue);
    _fds.push_back({.fd = clientFd, .events = POLLIN, .revents = 0});
    std::cout << "New client connected: fd = " << clientFd << std::endl;
}

void ClientHandler::removeClient(int fd, const std::string &reason)
{
    _world->removePlayer(fd);
    _parsers.erase(fd);

    _clients.erase(std::remove_if(_clients.begin(), _clients.end(), [fd](const std::unique_ptr<Client> &client) { return client->getFd() == fd; }), _clients.end());

    _fds.erase(std::remove_if(_fds.begin(), _fds.end(), [fd](const pollfd &pfd) { return pfd.fd == fd; }), _fds.end());
    std::cout << "Client disconnected: fd = " << fd << " reason=\"" << reason << "\"" << std::endl;
}

Client *ClientHandler::getClientByFd(int fd) const
{
    for (const auto &client : _clients)
        if (client->getFd() == fd)
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
