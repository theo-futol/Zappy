#pragma once

#include "Model/resourceset/ResourceSet.hpp"
#include "types/GridPosition.hpp"

namespace Zappy
{

/**
 * @class Tile
 * @brief A single grid cell holding the resources lying on its ground.
 */
class Tile
{
  public:
    Tile();

    /** @brief Grid position of the tile. @return The coordinates. */
    GridPosition position() const;

    /** @brief Sets the grid position. @param position New coordinates. */
    void setPosition(GridPosition position);

    /** @brief Mutable access to the ground resources. @return The resource set. */
    ResourceSet &resources();

    /** @brief Read-only access to the ground resources. @return The resource set. */
    const ResourceSet &resources() const;

  private:
    GridPosition _position; ///< Grid coordinates of the tile.
    ResourceSet _resources; ///< Resources lying on the ground.
};

} // namespace Zappy
