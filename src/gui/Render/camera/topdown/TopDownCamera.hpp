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
    void setViewport(int width, int height) override;

    /** @brief View matrix. @return The view transform. */
    Mat4 view() const override;

    /** @brief Projection matrix. @return The projection transform. */
    Mat4 projection() const override;

    /** @brief Current zoom level. @return The zoom factor. */
    float zoom() const override;

    /** @brief Sets the zoom level (clamped to [MinZoom, MaxZoom]). @param zoom New zoom factor. */
    void setZoom(float zoom);

    /**
     * @brief Multiplies the zoom by a factor (clamped); used for mouse-wheel zoom.
     * @param factor Multiplicative zoom step (>1 zooms in, <1 zooms out).
     */
    void zoomBy(float factor);

    /**
     * @brief Centers the camera on a world-space point (the pan).
     * @param x World X to look at.
     * @param y World Y to look at.
     */
    void setTarget(float x, float y);

    /**
     * @brief Pans the camera by a cursor delta in pixels (grab-and-drag).
     * @param dxPixels Horizontal cursor delta in pixels.
     * @param dyPixels Vertical cursor delta in pixels (screen-down positive).
     */
    void panPixels(float dxPixels, float dyPixels);

    /**
     * @brief Frames the whole map: centers on it and sets the zoom so it fits with a margin.
     * @param width Map width in tiles.
     * @param height Map height in tiles.
     */
    void fitToMap(int width, int height);

    /**
     * @brief Unprojects a viewport pixel to a world-space point (for picking).
     * @param px Pixel X in the world viewport (origin top-left).
     * @param py Pixel Y in the world viewport (origin top-left).
     * @param outX Receives the world X.
     * @param outY Receives the world Y.
     */
    void worldFromScreen(float px, float py, float &outX, float &outY) const;

    /** @brief World viewport width in pixels. @return The viewport width. */
    int viewportWidth() const;

  private:
    static constexpr float BaseHalfHeight = 5.0f; ///< Half view height in world units at zoom 1.
    static constexpr float MinZoom = 0.05f;       ///< Furthest zoom-out factor.
    static constexpr float MaxZoom = 20.0f;       ///< Closest zoom-in factor.

    int _width;     ///< Viewport width in pixels.
    int _height;    ///< Viewport height in pixels.
    float _zoom;    ///< Zoom factor (drives the future 2D-to-3D morph).
    float _aspect;  ///< Viewport aspect ratio (width / height).
    float _targetX; ///< World X the camera is centered on.
    float _targetY; ///< World Y the camera is centered on.
};

} // namespace Zappy
