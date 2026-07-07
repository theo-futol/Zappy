#include "VR/interaction/VRPointer.hpp"

#include <limits>

#include "types/WorldPoint.hpp"

namespace Zappy
{

VRPointer::Hit VRPointer::pickTile(const Ray &ray, const IProjection &projection, int mapWidth, int mapHeight, float pickRadius)
{
    Hit best{false, 0, 0, Vec3(0.0f)};
    float bestDistanceSq = pickRadius * pickRadius;

    for (int gridY = 0; gridY < mapHeight; ++gridY)
        for (int gridX = 0; gridX < mapWidth; ++gridX)
        {
            WorldPoint point = projection.toWorld(gridX, gridY);
            Vec3 toPoint = point.position - ray.origin;
            float along = glm::dot(toPoint, ray.direction);

            if (along < 0.0f)
                continue;

            Vec3 closest = ray.origin + ray.direction * along;
            Vec3 diff = closest - point.position;
            float distanceSq = glm::dot(diff, diff);

            if (distanceSq < bestDistanceSq)
            {
                bestDistanceSq = distanceSq;
                best = Hit{true, gridX, gridY, point.position};
            }
        }
    return best;
}

} // namespace Zappy
