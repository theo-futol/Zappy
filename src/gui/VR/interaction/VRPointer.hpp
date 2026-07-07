#pragma once

#include "interface/IProjection.hpp"
#include "types/Ray.hpp"
#include "types/Vec.hpp"

namespace Zappy
{

/**
 * @class VRPointer
 * @brief Turns a hand's aim-pose ray into the map tile it points at.
 *
 * The desktop 3D path picks tiles by projecting every tile to the screen and comparing to the
 * mouse cursor (RenderSystem::groundTile) -- a technique built around a single 2D screen. VR has
 * no such screen: a controller/hand's aim pose already *is* a 3D ray (VRInput::aimPose), so
 * picking here is a genuine ray-vs-tile distance test instead of a screen-space projection.
 */
class VRPointer
{
  public:
    /**
     * @struct Hit
     * @brief Result of pickTile(): the closest tile the ray passes near, if any.
     */
    struct Hit
    {
        bool found;      ///< False if no tile fell within the pick radius.
        int tileX;       ///< Hit tile column (valid only if found).
        int tileY;       ///< Hit tile row (valid only if found).
        Vec3 worldPoint; ///< The tile's world-space point (valid only if found).
    };

    /**
     * @brief Finds the grid tile whose world point is closest to a world-space ray.
     * @param ray World-space ray (typically a hand's aim pose).
     * @param projection Grid-to-world mapping for the active view (flat ground or torus).
     * @param mapWidth Map width in tiles.
     * @param mapHeight Map height in tiles.
     * @param pickRadius Maximum perpendicular distance from the ray to a tile's point, in world units.
     * @return The closest qualifying tile, or a Hit with found=false if none is within pickRadius.
     */
    static Hit pickTile(const Ray &ray, const IProjection &projection, int mapWidth, int mapHeight, float pickRadius);
};

} // namespace Zappy
