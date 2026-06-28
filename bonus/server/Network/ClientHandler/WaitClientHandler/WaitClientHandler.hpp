#pragma once
#include "../ClientHandler.hpp"
#include "../CommandParser/WaitCommandParser.hpp"

namespace zappy
{
    /// @brief ClientHandler variant that implements --wait-timeout serial-turn mode.
    ///
    /// When an AI player's command queue empties the game clock freezes globally and
    /// the server sends "waiting <ms>\n" to that player. The clock resumes when the
    /// player sends a command or the timeout expires (turn is skipped).
    class WaitClientHandler : public ClientHandler
    {
    public:
        WaitClientHandler(int port, int initialClientCapacity, World *world,
                          bool *serverIsRunning, int waitTimeoutMs);
        ~WaitClientHandler() override = default;

        void handleClients() override;

    protected:
        std::unique_ptr<CommandParser> makeParser(Client *client, World *world,
                                                  std::queue<std::string> *broadcastQueue) override;

    private:
        int _waitTimeoutMs;
        bool _gamePaused = false;
        std::chrono::steady_clock::time_point _pauseStartedAt;

        WaitCommandParser *getWaitParser(int fd) const;
        void checkAndNotifyIdle();
        void checkTimeouts();
        void unpause();
    };
} // namespace zappy
