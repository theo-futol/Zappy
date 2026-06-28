#pragma once

#include "interface/ICamera.hpp"
#include "types/Mat.hpp"
#include "types/Ray.hpp"
#include "types/Vec.hpp"

namespace Zappy
{

/**
 * @class OrbitCamera
 * @brief Perspective camera that orbits a target point (the 3D-mode camera).
 *
 * The camera position is derived from spherical coordinates around a target:
 * a distance (radius), a yaw (azimuth) and a pitch (elevation). The mouse drives
 * orbit (yaw/pitch) and dolly (distance); the target stays fixed unless reset.
 */
class OrbitCamera : public ICamera
{
  public:
    OrbitCamera();

    /**
     * @brief Updates the viewport size so the projection keeps the right aspect ratio.
     * @param width Viewport width in pixels.
     * @param height Viewport height in pixels.
     */
    void setViewport(int width, int height) override;

    /**
     * @brief Sets the point the camera orbits around and looks at.
     * @param target World-space target position.
     */
    void setTarget(const Vec3 &target);

    /**
     * @brief Sets the orbit angles directly (used to aim the view, e.g. for a POV).
     * @param yawDegrees Azimuth angle in degrees.
     * @param pitchDegrees Elevation angle in degrees (clamped to avoid flipping).
     */
    void setAngles(float yawDegrees, float pitchDegrees);

    /**
     * @brief Sets only the azimuth, leaving the elevation untouched (e.g. to lock a POV's heading).
     * @param yawDegrees Azimuth angle in degrees.
     */
    void setYaw(float yawDegrees);

    /**
     * @brief Unit direction the camera looks toward (from the eye to the target).
     * @return The forward view direction.
     */
    Vec3 forward() const;

    /**
     * @brief Rotates the camera around the target.
     * @param dYawDegrees Horizontal rotation step in degrees.
     * @param dPitchDegrees Vertical rotation step in degrees (clamped to avoid flipping).
     */
    void orbit(float dYawDegrees, float dPitchDegrees);

    /**
     * @brief Moves the camera closer to or further from the target.
     * @param factor Multiplicative distance step (<1 moves closer, >1 moves away); clamped.
     */
    void dolly(float factor);

    /**
     * @brief Frames a bounding sphere: centers on it and sets a distance that fits it in view.
     * @param center World-space center to look at.
     * @param radius Bounding-sphere radius to fit.
     */
    void frame(const Vec3 &center, float radius);

    /** @brief View matrix. @return The view transform. */
    Mat4 view() const override;

    /** @brief Projection matrix. @return The perspective transform. */
    Mat4 projection() const override;

    /** @brief Closeness factor (base distance / current distance). @return The zoom factor. */
    float zoom() const override;

    /**
     * @brief Builds the world-space ray going from the eye through a screen pixel.
     * @param screenX Cursor X in pixels (0 at the left).
     * @param screenY Cursor Y in pixels (0 at the top).
     * @return The picking ray (origin on the near plane, unit direction into the scene).
     */
    Ray rayThrough(double screenX, double screenY) const;

  private:
    /**
     * @brief Computes the camera world position from the spherical coordinates.
     * @return The eye position.
     */
    Vec3 position() const;

    static constexpr float Fov = 45.0f;          ///< Vertical field of view in degrees.
    static constexpr float Near = 0.1f;          ///< Near clip plane distance.
    static constexpr float Far = 5000.0f;         ///< Far clip plane distance.
    static constexpr float MinPitch = -89.0f;     ///< Lowest elevation (avoids gimbal flip at -90).
    static constexpr float MaxPitch = 89.0f;      ///< Highest elevation (avoids gimbal flip at +90).
    static constexpr float MinDistance = 0.5f;    ///< Closest orbit radius.
    static constexpr float MaxDistance = 3000.0f; ///< Furthest orbit radius.
    static constexpr float BaseDistance = 20.0f; ///< Default orbit radius (zoom reference).

    Vec3 _target;   ///< Point the camera orbits and looks at.
    float _distance; ///< Orbit radius.
    float _yaw;      ///< Azimuth angle in degrees.
    float _pitch;    ///< Elevation angle in degrees.
    float _aspect;   ///< Viewport aspect ratio (width / height).
    int _width;      ///< Viewport width in pixels (for unprojecting clicks).
    int _height;     ///< Viewport height in pixels (for unprojecting clicks).
};

} // namespace Zappy
