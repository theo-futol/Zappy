#pragma once

#include "interface/ICamera.hpp"
#include "types/Mat.hpp"
#include "types/Ray.hpp"
#include "types/Vec.hpp"

namespace Zappy
{

/**
 * @class FreeCamera
 * @brief Free-fly perspective camera: move with the keyboard, look with the mouse.
 *
 * The camera is a position plus a yaw/pitch heading. Mouse drag turns the view,
 * keyboard input moves it along its own axes (forward/strafe) and the world up.
 * Used as the 3D-mode camera when free-fly is selected, alongside the orbit camera.
 */
class FreeCamera : public ICamera
{
  public:
    FreeCamera();

    /**
     * @brief Updates the viewport size so the projection keeps the right aspect ratio.
     * @param width Viewport width in pixels.
     * @param height Viewport height in pixels.
     */
    void setViewport(int width, int height) override;

    /** @brief View matrix. @return The view transform. */
    Mat4 view() const override;

    /** @brief Projection matrix. @return The perspective transform. */
    Mat4 projection() const override;

    /** @brief Zoom level (unused for the free camera). @return Always 1. */
    float zoom() const override;

    /**
     * @brief Places the camera at a position looking along a direction (used on mode switch).
     * @param position World-space eye position.
     * @param forward Direction to look toward (need not be normalized).
     */
    void setPose(const Vec3 &position, const Vec3 &forward);

    /**
     * @brief Turns the view by the given angle deltas.
     * @param dYawDegrees Horizontal turn in degrees.
     * @param dPitchDegrees Vertical turn in degrees (clamped to avoid flipping).
     */
    void look(float dYawDegrees, float dPitchDegrees);

    /**
     * @brief Moves the camera along its own axes and the world up.
     * @param forwardAmount Distance along the view direction.
     * @param rightAmount Distance along the (horizontal) right axis.
     * @param upAmount Distance along the world up axis.
     */
    void move(float forwardAmount, float rightAmount, float upAmount);

    /**
     * @brief Builds the world-space pick ray through a screen pixel.
     * @param screenX Pixel X (window top-left origin).
     * @param screenY Pixel Y (window top-left origin).
     * @return The ray from the near plane toward the far plane.
     */
    Ray rayThrough(double screenX, double screenY) const;

  private:
    /** @brief Unit view direction from the yaw/pitch heading. @return The forward vector. */
    Vec3 forward() const;

    static constexpr float Fov = 45.0f;       ///< Vertical field of view in degrees.
    static constexpr float Near = 0.1f;       ///< Near clip plane distance.
    static constexpr float Far = 5000.0f;     ///< Far clip plane distance.
    static constexpr float MinPitch = -89.0f; ///< Lowest elevation (avoids flipping).
    static constexpr float MaxPitch = 89.0f;  ///< Highest elevation (avoids flipping).

    Vec3 _position; ///< Eye position in world space.
    float _yaw;     ///< Heading azimuth in degrees.
    float _pitch;   ///< Heading elevation in degrees.
    float _aspect;  ///< Viewport aspect ratio (width / height).
    int _width;     ///< Viewport width in pixels (for picking).
    int _height;    ///< Viewport height in pixels (for picking).
};

} // namespace Zappy
