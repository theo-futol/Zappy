#pragma once

#include "interface/IProjection.hpp"
#include "types/WorldPoint.hpp"

namespace Zappy
{

/**
 * @class PlanarProjection
 * @brief Flat top-down projection: maps grid cell (x, y) to world point (x, y, 0).
 *
 * The v1 implementation. A toroidal or morphing projection can be added beside it
 * without touching the renderers.
 */
class PlanarProjection : public IProjection
{
  public:
    /**
     * @brief Maps a grid cell to a point on the flat world plane.
     * @param gridX Column on the grid.
     * @param gridY Row on the grid.
     * @return The world point with an upward normal and a +X tangent.
     */
    WorldPoint toWorld(int gridX, int gridY) const override;
};

} // namespace Zappy
