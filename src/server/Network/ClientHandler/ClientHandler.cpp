#include "ClientHandler.hpp"

#define TIMEOUT 20000 // 20 seconds in milliseconds

namespace zappy
{
ClientHandler::ClientHandler() : _f(100), _lastResourceUpdate(std::chrono::steady_clock::now())
{
}

ClientHandler::~ClientHandler()
{
}

ClientHandler::ClientHandler(int port, int initialClientCapacity, int f) : _f(f), _lastResourceUpdate(std::chrono::steady_clock::now())
{
    _tcpSocket.create(AF_INET, SOCK_STREAM, 0);
    _tcpSocket.bind(port);
    _tcpSocket.listen();

    _fds.reserve(initialClientCapacity + 1);
    _fds.push_back({.fd = _tcpSocket.getFd(), .events = POLLIN, .revents = 0});
}

void ClientHandler::handleClients(void)
{
    while (true)
    {
        if (poll(_fds.data(), _fds.size(), TIMEOUT) < 0)
        {
            continue;
        }
        if (_fds[0].revents & POLLIN)
            addClient();
        auto now = std::chrono::steady_clock::now();
        if (now - _lastResourceUpdate >= std::chrono::milliseconds(TIMEOUT))
        {
            // TO DO: ADD RESSOURCE UPDATE LOGIC HERE
            _lastResourceUpdate = now;
        }

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
                    it->second->feed();
                    it->second->executeNext();
                }
            }
        }
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
} // namespace zappy
