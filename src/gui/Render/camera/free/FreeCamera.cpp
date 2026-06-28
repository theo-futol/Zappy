#include "Render/camera/free/FreeCamera.hpp"

#include <algorithm>
#include <cmath>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

namespace Zappy
{

FreeCamera::FreeCamera() : _position(0.0f, 5.0f, 10.0f), _yaw(0.0f), _pitch(-20.0f), _aspect(0.0f), _width(0), _height(0)
{
}

void FreeCamera::setViewport(int width, int height)
{
    if (!height)
        return;
    _width = width;
    _height = height;
    _aspect = static_cast<float>(width) / static_cast<float>(height);
}

Vec3 FreeCamera::forward() const
{
    float yaw = glm::radians(_yaw);
    float pitch = glm::radians(_pitch);

    return glm::normalize(Vec3(std::cos(pitch) * std::sin(yaw), std::sin(pitch), -std::cos(pitch) * std::cos(yaw)));
}

Mat4 FreeCamera::view() const
{
    return glm::lookAt(_position, _position + forward(), Vec3(0.0f, 1.0f, 0.0f));
}

Mat4 FreeCamera::projection() const
{
    return glm::perspective(glm::radians(Fov), _aspect, Near, Far);
}

float FreeCamera::zoom() const
{
    return 1.0f;
}

void FreeCamera::setPose(const Vec3 &position, const Vec3 &forward)
{
    Vec3 direction = glm::normalize(forward);

    _position = position;
    _yaw = glm::degrees(std::atan2(direction.x, -direction.z));
    _pitch = std::clamp(glm::degrees(std::asin(std::clamp(direction.y, -1.0f, 1.0f))), MinPitch, MaxPitch);
}

void FreeCamera::look(float dYawDegrees, float dPitchDegrees)
{
    _yaw += dYawDegrees;
    _pitch = std::clamp(_pitch + dPitchDegrees, MinPitch, MaxPitch);
}

void FreeCamera::move(float forwardAmount, float rightAmount, float upAmount)
{
    Vec3 direction = forward();
    Vec3 right = glm::normalize(glm::cross(direction, Vec3(0.0f, 1.0f, 0.0f)));

    _position += direction * forwardAmount + right * rightAmount + Vec3(0.0f, 1.0f, 0.0f) * upAmount;
}

Ray FreeCamera::rayThrough(double screenX, double screenY) const
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
