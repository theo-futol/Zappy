#include "WaitCommandParser.hpp"
#include "../../../Logger/Logger.hpp"

namespace zappy
{

WaitCommandParser::WaitCommandParser(Client *client, World *world, std::queue<std::string> *broadcastQueue, int waitTimeoutMs, bool *gamePaused)
    : CommandParser(client, world, broadcastQueue), _waitTimeoutMs(waitTimeoutMs), _gamePaused(gamePaused), _lastTurnEndedAt(std::chrono::steady_clock::now()),
      _notifiedAt(std::chrono::steady_clock::now())
{
}

bool WaitCommandParser::feed(std::vector<std::unique_ptr<Client>> &clients)
{
    bool result = CommandParser::feed(clients);
    // A response arrived while we were in the idle-wait cycle: clear the wait state.
    if (_turnEnded && !_commandQueue.empty())
    {
        _turnEnded = false;
        _notified = false;
    }
    return result;
}

bool WaitCommandParser::executeNext(std::vector<std::unique_ptr<Client>> &clients)
{
    // While the game clock is frozen only non-AI clients (handshake / GUI) may proceed.
    if (*_gamePaused && _client->getType() == ClientType::AI)
        return false;

    // First call for a freshly-connected AI client: treat it as "turn just ended" so
    // WaitClientHandler will send the initial "waiting" notification next iteration.
    if (_client->getType() == ClientType::AI && _commandQueue.empty() && !_turnEnded)
    {
        _lastTurnEndedAt = std::chrono::steady_clock::now();
        _turnEnded = true;
        return false;
    }

    bool consumed = CommandParser::executeNext(clients);

    if (consumed && _commandQueue.empty())
    {
        _lastTurnEndedAt = std::chrono::steady_clock::now();
        _turnEnded = true;
    }
    return consumed;
}

std::chrono::steady_clock::time_point WaitCommandParser::nextReadyAt() const
{
    if (_notified)
        return _notifiedAt + std::chrono::milliseconds(_waitTimeoutMs);
    if (_turnEnded)
        return _lastTurnEndedAt; // in the past → poll timeout = 0 → immediate wakeup
    return CommandParser::nextReadyAt();
}

std::chrono::steady_clock::time_point WaitCommandParser::commandQueueBase() const
{
    // When the queue is empty and we recorded a turn-end time, use that as the base
    // so any incoming response command is scheduled relative to game-time, not wall-time.
    if (_turnEnded && _commandQueue.empty())
        return _lastTurnEndedAt;
    return CommandParser::commandQueueBase();
}

bool WaitCommandParser::isIdle() const
{
    return _turnEnded;
}

bool WaitCommandParser::isWaitingForResponse() const
{
    return _notified;
}

void WaitCommandParser::markNotified()
{
    _notified = true;
    _notifiedAt = std::chrono::steady_clock::now();
}

void WaitCommandParser::resetWait()
{
    _turnEnded = false;
    _notified = false;
}

std::chrono::steady_clock::time_point WaitCommandParser::notifiedAt() const
{
    return _notifiedAt;
}

void WaitCommandParser::shiftDeadlines(std::chrono::milliseconds d)
{
    for (auto &cmd : _commandQueue)
        cmd.readyAt += d;
}

} // namespace zappy
