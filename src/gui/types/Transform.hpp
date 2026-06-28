#pragma once

#include "types/Vec.hpp"

namespace Zappy
{

/**
 * @struct Transform
 * @brief Placement of a drawable in world space.
 */
struct Transform
{
    Vec3 position;         ///< World-space position.
    float rotationDegrees; ///< Rotation around the surface normal, in degrees.
    float scale;           ///< Uniform scale factor.
};

} // namespace Zappy
