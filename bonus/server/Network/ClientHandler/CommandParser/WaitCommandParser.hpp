#pragma once
#include "CommandParser.hpp"

namespace zappy
{
    /// @brief CommandParser variant for --wait-timeout mode.
    ///
    /// After a player's command queue empties the server sends "waiting <ms>\n" and
    /// freezes the global game clock until the player responds or the timeout expires.
    class WaitCommandParser : public CommandParser
    {
    public:
        WaitCommandParser(Client *client, World *world, std::queue<std::string> *broadcastQueue,
                          int waitTimeoutMs, bool *gamePaused);
        ~WaitCommandParser() override = default;

        bool feed(std::vector<std::unique_ptr<Client>> &clients) override;
        bool executeNext(std::vector<std::unique_ptr<Client>> &clients) override;
        std::chrono::steady_clock::time_point nextReadyAt() const override;

        bool isIdle() const;
        bool isWaitingForResponse() const;
        void markNotified();
        void resetWait();
        std::chrono::steady_clock::time_point notifiedAt() const;
        void shiftDeadlines(std::chrono::milliseconds d);

    protected:
        std::chrono::steady_clock::time_point commandQueueBase() const override;

    private:
        int _waitTimeoutMs;
        bool *_gamePaused;

        bool _turnEnded = false;
        std::chrono::steady_clock::time_point _lastTurnEndedAt;

        bool _notified = false;
        std::chrono::steady_clock::time_point _notifiedAt;
    };
} // namespace zappy
