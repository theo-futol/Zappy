#include "Render/camera/topdown/TopDownCamera.hpp"

#include <algorithm>

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace Zappy
{

TopDownCamera::TopDownCamera() : _width(0), _height(0), _zoom(1.0f), _aspect(0.0f), _targetX(0.0f), _targetY(0.0f)
{
}

void TopDownCamera::setViewport(int width, int height)
{
    if (!height)
        return;
    _width = width;
    _height = height;
    _aspect = static_cast<float>(_width) / static_cast<float>(height);
}

Mat4 TopDownCamera::view() const
{
    return glm::lookAt(glm::vec3(_targetX, _targetY, 10.0f), glm::vec3(_targetX, _targetY, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

Mat4 TopDownCamera::projection() const
{
    float halfHeight = BaseHalfHeight / _zoom;
    float halfWidth = halfHeight * _aspect;

    return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, 0.1f, 100.0f);
}

float TopDownCamera::zoom() const
{
    return _zoom;
}

void TopDownCamera::setZoom(float zoom)
{
    _zoom = zoom;
}

void TopDownCamera::setTarget(float x, float y)
{
    _targetX = x;
    _targetY = y;
}

void TopDownCamera::fitToMap(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;
    setTarget((width - 1) / 2.0f, (height - 1) / 2.0f);

    float margin = 1.1f;
    float zoomHeight = (2.0f * BaseHalfHeight) / (height * margin);
    float zoomWidth = (2.0f * BaseHalfHeight * _aspect) / (width * margin);

    _zoom = std::min(zoomHeight, zoomWidth);
}

} // namespace Zappy
