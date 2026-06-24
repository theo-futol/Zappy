#pragma once

#include <exception>
#include <memory>
#include <string>

#include "Model/gamestate/GameState.hpp"
#include "Network/service/NetworkService.hpp"
#include "Render/rendersystem/RenderSystem.hpp"

namespace Zappy
{

/**
 * @class Core
 * @brief Top-level orchestrator: parses arguments, wires the subsystems and runs the loop.
 */
class Core
{
  public:
    /**
     * @class CoreException
     * @brief Top-level error; caught by main, which returns 84.
     */
    class CoreException : public std::exception
    {
      public:
        explicit CoreException(const std::string &message) : _message(message)
        {
        }

        const char *what() const noexcept override
        {
            return _message.c_str();
        }

      private:
        std::string _message; ///< Error description.
    };

    /**
     * @brief Parses the command-line arguments (-p port, -h machine).
     * @param argc Argument count.
     * @param argv Argument values.
     * @throws CoreException On invalid or missing arguments.
     */
    Core(int argc, char **argv);

    /**
     * @brief Connects to the server and builds the render system.
     * @throws CoreException On connection or render setup failure.
     */
    void init();

    /**
     * @brief Runs the main loop until the window closes or the game ends.
     * @throws CoreException On a fatal runtime error.
     */
    void run();

  private:
    /**
     * @brief Parses -p (port) and -h (host) from the command line.
     * @param argc Argument count.
     * @param argv Argument values.
     * @throws CoreException On an unknown flag, a missing value, or a missing port.
     */
    void parseArguments(int argc, char **argv);

    /**
     * @brief Parses and validates a TCP port value.
     * @param value Port string.
     * @return The port in [1, 65535].
     * @throws CoreException If the value is not a valid port.
     */
    static int parsePort(const std::string &value);

    static constexpr const char *USAGE = "USAGE: ./zappy_gui -p port -h machine"; ///< Command-line usage.

    GameState _state;                         ///< Single source of truth.
    std::unique_ptr<NetworkService> _network; ///< Server connection and pipeline.
    std::unique_ptr<RenderSystem> _render;    ///< Window and rendering.
    std::string _host;                        ///< Server hostname.
    int _port;                                ///< Server port.
};

} // namespace Zappy
