#pragma once

#include <string>

#include "types/Appearance.hpp"
#include "types/GridPosition.hpp"
#include "types/Orientation.hpp"

namespace Zappy
{

/**
 * @class IEntity
 * @brief Contract for any identified, positioned thing on the map.
 *
 * Implemented by every concrete entity through AMapEntity. The renderer and the
 * handlers manipulate entities through this interface only, so adding a new
 * entity type never forces a change in existing code.
 */
class IEntity
{
  public:
    virtual ~IEntity() = default;

    /**
     * @brief Open category of the entity.
     * @return The entity type string (e.g. "player", "egg").
     */
    virtual std::string getEntityType() const = 0;

    /**
     * @brief Protocol number within the entity's category.
     * @return The entity number.
     */
    virtual int number() const = 0;

    /**
     * @brief Current grid position.
     * @return The grid coordinates of the entity.
     */
    virtual GridPosition position() const = 0;

    /**
     * @brief Current facing orientation.
     * @return The entity orientation.
     */
    virtual Orientation orientation() const = 0;

    /**
     * @brief Moves the entity to a new grid position.
     * @param position New grid coordinates.
     */
    virtual void setPosition(GridPosition position) = 0;

    /**
     * @brief Sets the entity's facing orientation.
     * @param orientation New orientation.
     */
    virtual void setOrientation(Orientation orientation) = 0;

    /**
     * @brief View-independent visual description the entity declares for itself.
     * @return The appearance used by the renderer.
     */
    virtual Appearance appearance() const = 0;
};

} // namespace Zappy
