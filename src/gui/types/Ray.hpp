#pragma once

#include "types/Vec.hpp"

namespace Zappy
{

/**
 * @struct Ray
 * @brief A world-space half-line: an origin and a unit direction.
 *
 * Produced by unprojecting a screen pixel through the camera, then intersected
 * with the ground plane (or entities) to find what the cursor points at.
 */
struct Ray
{
    Vec3 origin;    ///< Starting point of the ray (world space).
    Vec3 direction; ///< Unit direction of the ray (world space).
};

} // namespace Zappy
