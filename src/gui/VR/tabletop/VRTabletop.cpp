#include "VR/tabletop/VRTabletop.hpp"

#include <algorithm>
#include <cmath>

#include <glm/ext/matrix_transform.hpp>

namespace Zappy
{

VRTabletop::VRTabletop() : _center(0.0f, 0.9f, -0.6f), _scale(0.1f), _mapWidth(0), _mapHeight(0)
{
}

void VRTabletop::autoFrame(int mapWidth, int mapHeight)
{
    _mapWidth = mapWidth;
    _mapHeight = mapHeight;

    float largest = static_cast<float>(std::max(mapWidth, mapHeight));

    _scale = largest > 0.0f ? std::clamp(TargetFootprintMeters / largest, MinScale, MaxScale) : _scale;
}

void VRTabletop::zoomBy(float factor)
{
    _scale = std::clamp(_scale * factor, MinScale, MaxScale);
}

Mat4 VRTabletop::tableModel() const
{
    Vec3 gridCenter(static_cast<float>(_mapWidth - 1) * 0.5f, 0.0f, static_cast<float>(_mapHeight - 1) * 0.5f);
    Mat4 model = glm::translate(Mat4(1.0f), _center);

    model = glm::scale(model, Vec3(_scale, _scale, _scale));
    model = glm::translate(model, -gridCenter);
    return model;
}

VRTabletop::Hit VRTabletop::pickTile(const Ray &ray) const
{
    Hit miss{false, 0, 0};

    if (std::fabs(ray.direction.y) < 1e-6f)
        return miss;

    float distance = (_center.y - ray.origin.y) / ray.direction.y;

    if (distance < 0.0f)
        return miss;

    Vec3 hitWorld = ray.origin + ray.direction * distance;
    Vec3 gridCenter(static_cast<float>(_mapWidth - 1) * 0.5f, 0.0f, static_cast<float>(_mapHeight - 1) * 0.5f);
    Vec3 gridLocal = (hitWorld - _center) / _scale + gridCenter;
    int tileX = static_cast<int>(std::lround(gridLocal.x));
    int tileY = static_cast<int>(std::lround(gridLocal.z));

    if (tileX < 0 || tileX >= _mapWidth || tileY < 0 || tileY >= _mapHeight)
        return miss;
    return Hit{true, tileX, tileY};
}

} // namespace Zappy
