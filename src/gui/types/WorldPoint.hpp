#pragma once

#include "types/Vec.hpp"

namespace Zappy
{

/**
 * @struct WorldPoint
 * @brief A point on the rendered world surface, produced by IProjection.
 */
struct WorldPoint
{
    Vec3 position; ///< World-space position.
    Vec3 normal;   ///< Surface normal at this point.
    Vec3 tangent;  ///< Surface tangent at this point.
};

} // namespace Zappy
