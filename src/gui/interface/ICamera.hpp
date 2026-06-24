#pragma once

#include "types/Mat.hpp"

namespace Zappy
{

/**
 * @class ICamera
 * @brief Contract for a camera providing view and projection matrices.
 */
class ICamera
{
  public:
    virtual ~ICamera() = default;

    /**
     * @brief View matrix for the current frame.
     * @return The view transform.
     */
    virtual Mat4 view() const = 0;

    /**
     * @brief Projection matrix for the current frame.
     * @return The projection transform.
     */
    virtual Mat4 projection() const = 0;

    /**
     * @brief Current zoom level.
     * @return The zoom factor (will drive the future 2D-to-3D morph).
     */
    virtual float zoom() const = 0;
};

} // namespace Zappy
