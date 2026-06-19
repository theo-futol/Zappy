#include "ClientHandler.hpp"

static constexpr int RESOURCE_INTERVAL_MS = 20000;
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
        auto deadline = _lastResourceUpdate + std::chrono::milliseconds(RESOURCE_INTERVAL_MS);
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
            throw ServerException("poll failed: " + std::string(strerror(errno)));
        }
        if (_fds[0].revents & POLLIN)
            addClient();

        now = std::chrono::steady_clock::now();
        if (now - _lastResourceUpdate >= std::chrono::milliseconds(RESOURCE_INTERVAL_MS))
        {
            _world->ressourcePassiveGeneration();
            _lastResourceUpdate = now;
        }
        if (now - _lastFoodUpdate >= foodIntervalMs)
        {
            _world->foodCheck();
            _lastFoodUpdate = now;
        }

        clientEventHandling();
        if (_world->checkWinningCondition())
            *_serverIsRunning = false;
        broadcastGuiInfo();
    }
}

void ClientHandler::clientEventHandling()
{
    for (size_t i = 1; i < _fds.size(); i++)
    {
        if (_fds[i].revents & POLLHUP)
        {
            removeClient(_fds[i].fd);
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
                    removeClient(_fds[i].fd);
                    i--;
                    continue;
                }
            }
        }
    }
    for (auto &[fd, parser] : _parsers)
    {
        while (parser->hasPending())
            parser->executeNext();
    }
}

void ClientHandler::addClient()
{
    int clientFd = _tcpSocket.accept();
    if (clientFd < 0)
        return;
    send(clientFd, "WELCOME\n", 8, 0);
    _clients.push_back(std::make_unique<Client>(clientFd));
    _parsers[clientFd] = std::make_unique<CommandParser>(_clients.back().get(), _f);
    _fds.push_back({.fd = clientFd, .events = POLLIN, .revents = 0});
}

void ClientHandler::removeClient(int fd)
{
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

void ClientHandler::broadcastGuiInfo()
{
    while (!_broadcastQueue.empty())
    {
        std::string message = _broadcastQueue.front();
        _broadcastQueue.pop();
        for (const auto &client : _clients)
            if (client->getType() == ClientType::GRAPHIC)
                send(client->getFd(), message.c_str(), message.size(), 0);
    }
}

std::queue<std::string> &ClientHandler::getBroadcastQueue()
{
    return _broadcastQueue;
}
} // namespace zappy
