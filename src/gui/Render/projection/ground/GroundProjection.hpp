#pragma once

#include "interface/IProjection.hpp"
#include "types/WorldPoint.hpp"

namespace Zappy
{

/**
 * @class GroundProjection
 * @brief 3D-mode projection: maps grid cell (x, y) to world point (x, 0, y).
 *
 * The map lies flat on the XZ ground plane with Y pointing up, so 3D models stand
 * on it. A toroidal projection can replace it later without touching the renderers.
 */
class GroundProjection : public IProjection
{
  public:
    /**
     * @brief Maps a grid cell to a point on the flat ground plane.
     * @param gridX Column on the grid.
     * @param gridY Row on the grid.
     * @return The world point on the XZ plane with an upward normal and a +X tangent.
     */
    WorldPoint toWorld(int gridX, int gridY) const override;
};

} // namespace Zappy
