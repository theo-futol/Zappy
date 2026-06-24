#pragma once

#include "Model/gamestate/GameState.hpp"
#include "interface/IProjection.hpp"
#include "types/Mat.hpp"

namespace Zappy
{

/**
 * @struct RenderContext
 * @brief Per-frame data handed to every renderer.
 */
struct RenderContext
{
    const GameState &state;     ///< Authoritative state to read.
    Mat4 view;                  ///< Camera view matrix.
    Mat4 projection;            ///< Camera projection matrix.
    const IProjection &mapping; ///< Grid-to-world mapping.
};

} // namespace Zappy
