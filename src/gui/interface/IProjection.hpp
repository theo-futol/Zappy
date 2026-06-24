#pragma once

#include "types/WorldPoint.hpp"

namespace Zappy
{

/**
 * @class IProjection
 * @brief Contract mapping grid coordinates to a point on the rendered world surface.
 *
 * Planar in v1; a toroidal or morphing projection can be added later without
 * touching the renderers. This is the 2D-to-3D seam.
 */
class IProjection
{
  public:
    virtual ~IProjection() = default;

    /**
     * @brief Maps a grid cell to its world-space surface point.
     * @param gridX Column on the grid.
     * @param gridY Row on the grid.
     * @return The world-space position, normal and tangent at that cell.
     */
    virtual WorldPoint toWorld(int gridX, int gridY) const = 0;
};

} // namespace Zappy
