#include "Render/projection/torus/TorusProjection.hpp"

#include <cmath>

#include <glm/geometric.hpp>

namespace Zappy
{

TorusProjection::TorusProjection() : _width(1), _height(1), _major(DefaultMajor), _minor(DefaultMinor)
{
}

void TorusProjection::setGridSize(int width, int height)
{
    _width = (width > 0) ? width : 1;
    _height = (height > 0) ? height : 1;
}

void TorusProjection::setRadii(float major, float minor)
{
    _major = major;
    _minor = minor;
}

float TorusProjection::major() const
{
    return _major;
}

float TorusProjection::minor() const
{
    return _minor;
}

WorldPoint TorusProjection::toWorld(int gridX, int gridY) const
{
    float u = Tau * static_cast<float>(gridX) / static_cast<float>(_width);
    float v = Tau * static_cast<float>(gridY) / static_cast<float>(_height);
    float ring = _major + _minor * std::cos(v);
    Vec3 position(ring * std::cos(u), _minor * std::sin(v), ring * std::sin(u));
    Vec3 center(_major * std::cos(u), 0.0f, _major * std::sin(u));
    Vec3 normal = glm::normalize(center - position);
    Vec3 tangent(-std::sin(u), 0.0f, std::cos(u));

    return WorldPoint{position, normal, tangent};
}

} // namespace Zappy
