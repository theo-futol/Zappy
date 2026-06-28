#include "Render/camera/orbit/OrbitCamera.hpp"

#include <algorithm>
#include <cmath>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>

namespace Zappy
{

OrbitCamera::OrbitCamera() : _target(0.0f, 0.0f, 0.0f), _distance(BaseDistance), _yaw(45.0f), _pitch(30.0f), _aspect(0.0f), _width(0), _height(0)
{
}

void OrbitCamera::setViewport(int width, int height)
{
    if (!height)
        return;
    _width = width;
    _height = height;
    _aspect = static_cast<float>(width) / static_cast<float>(height);
}

void OrbitCamera::setTarget(const Vec3 &target)
{
    _target = target;
}

void OrbitCamera::setAngles(float yawDegrees, float pitchDegrees)
{
    _yaw = yawDegrees;
    _pitch = std::clamp(pitchDegrees, MinPitch, MaxPitch);
}

void OrbitCamera::setYaw(float yawDegrees)
{
    _yaw = yawDegrees;
}

Vec3 OrbitCamera::forward() const
{
    return glm::normalize(_target - position());
}

void OrbitCamera::orbit(float dYawDegrees, float dPitchDegrees)
{
    _yaw += dYawDegrees;
    _pitch = std::clamp(_pitch + dPitchDegrees, MinPitch, MaxPitch);
}

void OrbitCamera::dolly(float factor)
{
    _distance = std::clamp(_distance * factor, MinDistance, MaxDistance);
}

void OrbitCamera::frame(const Vec3 &center, float radius)
{
    _target = center;
    _distance = std::clamp(radius / std::sin(glm::radians(Fov * 0.5f)), MinDistance, MaxDistance);
}

Vec3 OrbitCamera::position() const
{
    float yaw = glm::radians(_yaw);
    float pitch = glm::radians(_pitch);
    Vec3 offset(_distance * std::cos(pitch) * std::sin(yaw), _distance * std::sin(pitch), _distance * std::cos(pitch) * std::cos(yaw));

    return _target + offset;
}

Mat4 OrbitCamera::view() const
{
    return glm::lookAt(position(), _target, Vec3(0.0f, 1.0f, 0.0f));
}

Mat4 OrbitCamera::projection() const
{
    return glm::perspective(glm::radians(Fov), _aspect, Near, Far);
}

float OrbitCamera::zoom() const
{
    return BaseDistance / _distance;
}

Ray OrbitCamera::rayThrough(double screenX, double screenY) const
{
    float ndcX = 2.0f * static_cast<float>(screenX) / static_cast<float>(_width) - 1.0f;
    float ndcY = 1.0f - 2.0f * static_cast<float>(screenY) / static_cast<float>(_height);
    Mat4 inverse = glm::inverse(projection() * view());
    Vec4 nearPoint = inverse * Vec4(ndcX, ndcY, -1.0f, 1.0f);
    Vec4 farPoint = inverse * Vec4(ndcX, ndcY, 1.0f, 1.0f);
    Vec3 origin = Vec3(nearPoint) / nearPoint.w;
    Vec3 target = Vec3(farPoint) / farPoint.w;

    return Ray{origin, glm::normalize(target - origin)};
}

} // namespace Zappy
