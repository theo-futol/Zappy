#pragma once

#include "interface/ICamera.hpp"
#include "types/Mat.hpp"

namespace Zappy
{

/**
 * @class TopDownCamera
 * @brief Orthographic top-down camera (the v1 camera).
 */
class TopDownCamera : public ICamera
{
  public:
    TopDownCamera();

    /**
     * @brief Updates the viewport size so the projection keeps the right aspect ratio.
     * @param width Viewport width in pixels.
     * @param height Viewport height in pixels.
     */
    void setViewport(int width, int height);

    /** @brief View matrix. @return The view transform. */
    Mat4 view() const override;

    /** @brief Projection matrix. @return The projection transform. */
    Mat4 projection() const override;

    /** @brief Current zoom level. @return The zoom factor. */
    float zoom() const override;

    /** @brief Sets the zoom level. @param zoom New zoom factor. */
    void setZoom(float zoom);

    /**
     * @brief Centers the camera on a world-space point (the pan).
     * @param x World X to look at.
     * @param y World Y to look at.
     */
    void setTarget(float x, float y);

    /**
     * @brief Frames the whole map: centers on it and sets the zoom so it fits with a margin.
     * @param width Map width in tiles.
     * @param height Map height in tiles.
     */
    void fitToMap(int width, int height);

  private:
    static constexpr float BaseHalfHeight = 5.0f; ///< Half view height in world units at zoom 1.

    int _width;     ///< Viewport width in pixels.
    int _height;    ///< Viewport height in pixels.
    float _zoom;    ///< Zoom factor (drives the future 2D-to-3D morph).
    float _aspect;  ///< Viewport aspect ratio (width / height).
    float _targetX; ///< World X the camera is centered on.
    float _targetY; ///< World Y the camera is centered on.
};

} // namespace Zappy
