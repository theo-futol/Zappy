#include "VR/rig/VRRig.hpp"

#include <cmath>

#include <glm/ext/matrix_transform.hpp>
#include <glm/matrix.hpp>

namespace Zappy
{

namespace
{

constexpr float MoveDeadzone = 0.15f; ///< Thumbstick magnitude below which locomotion input is ignored.

} // namespace

VRRig::VRRig() : _position(0.0f, 0.0f, 0.0f), _yaw(0.0f)
{
}

void VRRig::updateLocomotion(const Vec2 &stickMove, float headsetYawDegrees, float metersPerSecond, float dSeconds)
{
    if (glm::length(stickMove) < MoveDeadzone)
        return;

    float totalYaw = glm::radians(_yaw + headsetYawDegrees);
    Vec3 forward(-std::sin(totalYaw), 0.0f, -std::cos(totalYaw));
    Vec3 right(std::cos(totalYaw), 0.0f, -std::sin(totalYaw));
    Vec3 delta = (forward * stickMove.y + right * stickMove.x) * metersPerSecond * dSeconds;

    _position += delta;
}

void VRRig::turn(float dYawDegrees)
{
    _yaw += dYawDegrees;
}

Vec3 VRRig::position() const
{
    return _position;
}

float VRRig::yaw() const
{
    return _yaw;
}

void VRRig::setPose(const Vec3 &position, float yawDegrees)
{
    _position = position;
    _yaw = yawDegrees;
}

Mat4 VRRig::transform() const
{
    return glm::translate(Mat4(1.0f), _position) * glm::rotate(Mat4(1.0f), glm::radians(_yaw), Vec3(0.0f, 1.0f, 0.0f));
}

float VRRig::yawFromOrientation(const Quat &orientation)
{
    float sinYaw = 2.0f * (orientation.w * orientation.y + orientation.x * orientation.z);
    float cosYaw = 1.0f - 2.0f * (orientation.x * orientation.x + orientation.y * orientation.y);

    return glm::degrees(std::atan2(sinYaw, cosYaw));
}

Mat4 VRRig::eyeView(const VRSession::EyeView &eye) const
{
    Mat4 eyeLocal = glm::translate(Mat4(1.0f), eye.position) * glm::mat4_cast(eye.orientation);
    Mat4 eyeWorld = transform() * eyeLocal;

    return glm::inverse(eyeWorld);
}

Mat4 VRRig::eyeProjection(const VRSession::EyeView &eye, float nearPlane, float farPlane)
{
    float tanLeft = std::tan(eye.angleLeft);
    float tanRight = std::tan(eye.angleRight);
    float tanUp = std::tan(eye.angleUp);
    float tanDown = std::tan(eye.angleDown);
    float tanWidth = tanRight - tanLeft;
    float tanHeight = tanUp - tanDown;

    Mat4 proj(0.0f);

    proj[0][0] = 2.0f / tanWidth;
    proj[1][1] = 2.0f / tanHeight;
    proj[2][0] = (tanRight + tanLeft) / tanWidth;
    proj[2][1] = (tanUp + tanDown) / tanHeight;
    proj[2][2] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    proj[2][3] = -1.0f;
    proj[3][2] = -(2.0f * farPlane * nearPlane) / (farPlane - nearPlane);
    return proj;
}

} // namespace Zappy
