#pragma once

#include <exception>
#include <memory>
#include <string>

#include "Graphics/context/GraphicsContext.hpp"
#include "Graphics/window/Window.hpp"
#include "Menu/mainmenu/MainMenu.hpp"
#include "Model/gamestate/GameState.hpp"
#include "Network/service/NetworkService.hpp"
#include "Render/rendersystem/RenderSystem.hpp"
#include "types/RenderMode.hpp"

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
     * @brief Runs the application: alternates the main menu and a game session until the user quits.
     * @throws CoreException On a fatal runtime error.
     */
    void run();

  private:
    /**
     * @brief Connects and runs one game session with the current host/port/mode.
     * @param error Receives a message if the connection failed (caller shows it in the menu).
     * @return True to return to the menu (MENU button, dropped server or failed connect), false to quit.
     */
    bool runSession(std::string &error);

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

    /**
     * @brief Parses the display mode value of the -m flag.
     * @param value Mode string ("2d" or "3d").
     * @return The matching RenderMode.
     * @throws CoreException If the value is neither "2d" nor "3d".
     */
    static RenderMode parseMode(const std::string &value);

    static constexpr const char *USAGE = "USAGE: ./zappy_gui -p port -h machine [-m 2d|3d|torus] [--vr]"; ///< Command-line usage.
    static constexpr int WindowWidth = 1280;                                                 ///< Initial window width.
    static constexpr int WindowHeight = 720;                                                 ///< Initial window height.
    static constexpr const char *WindowTitle = "Zappy";                                      ///< Window title.

    GameState _state;                         ///< Single source of truth.
    std::unique_ptr<Window> _window;          ///< Persistent window/context, shared by the menu and the game.
    std::unique_ptr<GraphicsContext> _context; ///< Persistent OpenGL state, shared by the menu and the game.
    std::unique_ptr<MainMenu> _menu;          ///< Persistent configuration menu (shares the window).
    std::unique_ptr<NetworkService> _network; ///< Server connection and pipeline.
    std::unique_ptr<RenderSystem> _render;    ///< Window and rendering.
    std::string _host;                        ///< Server hostname.
    int _port;                                ///< Server port.
    RenderMode _mode;                         ///< Display mode (-m), defaults to 2D.
    bool _vrRequested;                        ///< Whether to attempt a VR session (--vr), regardless of whether it's available.
};

} // namespace Zappy
