#pragma once

#include <string>

#include "types/RenderMode.hpp"

namespace Zappy
{

/**
 * @enum MenuResult
 * @brief Outcome of a main-menu session.
 */
enum class MenuResult
{
    Connect, ///< The user validated and wants to connect with the chosen settings.
    Quit     ///< The user asked to quit the application.
};

/**
 * @struct MenuConfig
 * @brief Connection settings produced by the main menu.
 */
struct MenuConfig
{
    MenuResult result;  ///< What the user decided (connect or quit).
    std::string host;   ///< Server hostname or IP entered by the user.
    int port;           ///< Server TCP port entered by the user.
    RenderMode mode;    ///< Chosen display mode (2D or 3D).
    bool vr;            ///< Whether to attempt a VR session on top of that mode.
};

} // namespace Zappy
