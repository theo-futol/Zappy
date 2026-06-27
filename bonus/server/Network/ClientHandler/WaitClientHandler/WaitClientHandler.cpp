#include "WaitClientHandler.hpp"
#include "../../../Logger/Logger.hpp"
#include "../../../ServerException/ServerException.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <sys/socket.h>

static constexpr int RESOURCE_INTERVAL_TIME_UNITS = 20;
static constexpr int CYCLE_TO_DIE = 126;

namespace zappy
{

WaitClientHandler::WaitClientHandler(int port, int initialClientCapacity, World *world, bool *serverIsRunning, int waitTimeoutMs)
    : ClientHandler(port, initialClientCapacity, world, serverIsRunning), _waitTimeoutMs(waitTimeoutMs)
{
}

std::unique_ptr<CommandParser> WaitClientHandler::makeParser(Client *client, World *world, std::queue<std::string> *broadcastQueue)
{
    return std::make_unique<WaitCommandParser>(client, world, broadcastQueue, _waitTimeoutMs, &_gamePaused);
}

WaitCommandParser *WaitClientHandler::getWaitParser(int fd) const
{
    auto it = _parsers.find(fd);
    if (it == _parsers.end())
        return nullptr;
    return static_cast<WaitCommandParser *>(it->second.get());
}

void WaitClientHandler::checkAndNotifyIdle()
{
    if (_gamePaused)
        return;
    for (auto &[fd, parser] : _parsers)
    {
        WaitCommandParser *wp = getWaitParser(fd);
        if (!wp || !wp->isIdle() || wp->isWaitingForResponse())
            continue;
        Client *client = getClientByFd(fd);
        if (client)
        {
            std::string msg = "waiting " + std::to_string(_waitTimeoutMs) + "\n";
            send(client->getFd(), msg.c_str(), msg.size(), MSG_NOSIGNAL);
            Logger::log("060", "WaitMode : turn notification sent", {{"player_id", std::to_string(client->getPlayerId())}, {"timeout_ms", std::to_string(_waitTimeoutMs)}});
        }
        wp->markNotified();
        _gamePaused = true;
        _pauseStartedAt = std::chrono::steady_clock::now();
        break;
    }
}

void WaitClientHandler::checkTimeouts()
{
    if (!_gamePaused)
        return;
    auto now = std::chrono::steady_clock::now();
    for (auto &[fd, parser] : _parsers)
    {
        WaitCommandParser *wp = getWaitParser(fd);
        if (!wp || !wp->isWaitingForResponse())
            continue;
        if (!wp->isIdle())
        {
            // Player responded: a command arrived and cleared the idle state.
            Client *client = getClientByFd(fd);
            Logger::log("061", "WaitMode : response received, resuming", {{"player_id", std::to_string(client ? client->getPlayerId() : -1)}});
            unpause();
            return;
        }
        if (now >= wp->nextReadyAt())
        {
            // Timeout elapsed: skip this player's turn.
            Client *client = getClientByFd(fd);
            Logger::log("062", "WaitMode : timeout, skipping turn", {{"player_id", std::to_string(client ? client->getPlayerId() : -1)}});
            wp->resetWait();
            unpause();
            return;
        }
        return; // still waiting, nothing to do yet
    }
    // Paused but no parser is waiting (notified client disconnected) — resume.
    Logger::log("064", "WaitMode : no waiting parser found while paused, resuming");
    unpause();
}

void WaitClientHandler::unpause()
{
    auto now = std::chrono::steady_clock::now();
    auto pauseDuration = std::chrono::duration_cast<std::chrono::milliseconds>(now - _pauseStartedAt);
    _lastResourceUpdate += pauseDuration;
    _lastFoodUpdate += pauseDuration;
    for (auto &[fd, parser] : _parsers)
    {
        WaitCommandParser *wp = getWaitParser(fd);
        if (wp)
            wp->shiftDeadlines(pauseDuration);
    }
    _gamePaused = false;
    Logger::log("063", "WaitMode : clock resumed", {{"pause_ms", std::to_string(pauseDuration.count())}});
}

void WaitClientHandler::handleClients()
{
    Logger::log("000", "Poll : initialized successfully (wait-timeout mode)");
    while (*_serverIsRunning)
    {
        checkAndNotifyIdle();

        auto now = std::chrono::steady_clock::now();
        auto foodIntervalMs = std::chrono::milliseconds(CYCLE_TO_DIE * 1000 / _world->getTimeUnit());
        auto resourceIntervalMs = std::chrono::milliseconds(RESOURCE_INTERVAL_TIME_UNITS * 1000 / _world->getTimeUnit());

        std::chrono::steady_clock::time_point deadline;
        if (_gamePaused)
        {
            deadline = std::chrono::steady_clock::time_point::max();
            for (auto &[fd, parser] : _parsers)
            {
                WaitCommandParser *wp = getWaitParser(fd);
                if (wp && wp->isWaitingForResponse())
                {
                    auto t = wp->nextReadyAt();
                    if (t < deadline)
                        deadline = t;
                }
            }
            if (deadline == std::chrono::steady_clock::time_point::max())
                deadline = now + std::chrono::milliseconds(10);
        }
        else
        {
            deadline = _lastResourceUpdate + resourceIntervalMs;
            auto foodDeadline = _lastFoodUpdate + foodIntervalMs;
            if (foodDeadline < deadline)
                deadline = foodDeadline;
            for (auto &[fd, parser] : _parsers)
            {
                auto t = parser->nextReadyAt();
                if (t < deadline)
                    deadline = t;
            }
        }

        int64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();
        int timeout = static_cast<int>(std::max(static_cast<int64_t>(0), ms));

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

        // Feed incoming data and run executeNext (WaitCommandParser blocks AI execution while paused).
        clientEventHandling();

        // Resolve pause if a response arrived or if the timeout expired.
        checkTimeouts();

        if (!_gamePaused)
        {
            now = std::chrono::steady_clock::now();
            if (now - _lastResourceUpdate >= resourceIntervalMs)
            {
                _world->resourcePassiveGeneration();
                _lastResourceUpdate = now;
            }
            if (now - _lastFoodUpdate >= foodIntervalMs)
            {
                for (int id : _world->foodCheck())
                {
                    Client *client = getClientByPlayerId(id);
                    if (client)
                        client->setType(ClientType::DEAD);
                }
                _lastFoodUpdate = now;
            }
        }

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

} // namespace zappy
