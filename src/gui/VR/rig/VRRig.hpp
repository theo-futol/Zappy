#pragma once

#include "VR/session/VRSession.hpp"
#include "types/Mat.hpp"
#include "types/Vec.hpp"

namespace Zappy
{

/**
 * @class VRRig
 * @brief Combines the headset's tracked head pose with a locomotion offset into per-eye matrices.
 *
 * Deliberately not an ICamera: VR needs two asymmetric-FOV eye matrices per frame (one per
 * VRSession::EyeView), which ICamera's single view()/projection() contract cannot express. Shaped
 * like FreeCamera (position + yaw, moved/turned), but kept separate since it feeds two eyes.
 */
class VRRig
{
  public:
    VRRig();

    /**
     * @brief Moves the play-space origin, relative to the direction the player is looking.
     * @param stickMove Thumbstick axes (x = strafe, y = forward), already deadzone-filtered.
     * @param headsetYawDegrees Current headset yaw, so movement follows where the user looks.
     * @param metersPerSecond Locomotion speed.
     * @param dSeconds Frame delta time in seconds.
     */
    void updateLocomotion(const Vec2 &stickMove, float headsetYawDegrees, float metersPerSecond, float dSeconds);

    /**
     * @brief Turns the play-space origin by a yaw step (snap-turn, or per-frame delta for smooth-turn).
     * @param dYawDegrees Yaw delta to apply immediately, in degrees.
     */
    void turn(float dYawDegrees);

    /** @brief Play-space origin position in world space. @return The rig position. */
    Vec3 position() const;

    /** @brief Play-space origin yaw in world space, degrees. @return The rig yaw. */
    float yaw() const;

    /**
     * @brief Repositions the rig directly (e.g. when switching view modes or recentering).
     * @param position New world-space rig position.
     * @param yawDegrees New world-space rig yaw, in degrees.
     */
    void setPose(const Vec3 &position, float yawDegrees);

    /**
     * @brief Combines the rig transform with a headset eye pose into a world-space view matrix.
     * @param eye Eye pose/fov from VRSession::locateViews().
     * @return The view matrix (world-to-eye).
     */
    Mat4 eyeView(const VRSession::EyeView &eye) const;

    /**
     * @brief Builds the eye's off-axis (asymmetric-frustum) projection matrix from its raw fov.
     * @param eye Eye pose/fov from VRSession::locateViews().
     * @param nearPlane Near clip distance.
     * @param farPlane Far clip distance.
     * @return The projection matrix.
     */
    static Mat4 eyeProjection(const VRSession::EyeView &eye, float nearPlane, float farPlane);

    /**
     * @brief World-space transform of the play-space origin (translation * yaw rotation).
     * @return The rig transform, composed with a headset-local eye pose to place it in the world.
     */
    Mat4 transform() const;

    /**
     * @brief Extracts the heading (rotation around world Y) from an orientation quaternion.
     *
     * Assumes the same right-handed, Y-up, -Z-forward convention as the rest of the codebase
     * (see FreeCamera::forward()/OrbitCamera::forward()), which OpenXR poses also follow.
     * @param orientation Orientation to extract the heading from (e.g. a headset eye pose).
     * @return The heading in degrees, matching this class's own yaw() convention.
     */
    static float yawFromOrientation(const Quat &orientation);

  private:
    Vec3 _position; ///< World-space position of the VR play-space origin.
    float _yaw;      ///< World-space yaw (degrees) of the VR play-space origin.
};

} // namespace Zappy
