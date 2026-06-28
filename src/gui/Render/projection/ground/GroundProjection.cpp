#include "Render/projection/ground/GroundProjection.hpp"

namespace Zappy
{

WorldPoint GroundProjection::toWorld(int gridX, int gridY) const
{
    Vec3 position(static_cast<float>(gridX), 0.0f, static_cast<float>(gridY));
    Vec3 normal(0.0f, 1.0f, 0.0f);
    Vec3 tangent(1.0f, 0.0f, 0.0f);

    return WorldPoint{position, normal, tangent};
}

} // namespace Zappy
