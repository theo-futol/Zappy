#pragma once

namespace zappy
{
    /// @brief Role of a connected client, which decides how the server treats its messages.
    enum class ClientType
    {
        GRAPHIC, ///< A graphical viewer: receives world-state updates, sends no game actions.
        AI,      ///< An AI player: drives a player in the world via commands.
        DEAD,    ///< Connection that should be closed and cleaned up.
        UNKNOWN  ///< Newly accepted connection whose role is not yet identified.
    };
} // namespace zappy