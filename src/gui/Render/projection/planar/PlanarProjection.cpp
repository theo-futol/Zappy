#include "Render/projection/planar/PlanarProjection.hpp"

namespace Zappy
{

WorldPoint PlanarProjection::toWorld(int gridX, int gridY) const
{
    Vec3 position(static_cast<float>(gridX), static_cast<float>(gridY), 0.0f);
    Vec3 normal(0.0f, 0.0f, 1.0f);
    Vec3 tangent(1.0f, 0.0f, 0.0f);

    return WorldPoint{position, normal, tangent};
}

} // namespace Zappy
