#pragma once

#include <string>
#include <vector>

#include "interface/IEntity.hpp"
#include "types/GridPosition.hpp"
#include "types/Orientation.hpp"

namespace Zappy
{

/**
 * @class AMapEntity
 * @brief Common base for map entities: stores identity, position and orientation.
 *
 * Concrete entities only provide what distinguishes them (entity type, appearance and
 * their own data). Number, position and orientation handling live here so they
 * are never duplicated.
 */
class AMapEntity : public IEntity
{
  public:
    /**
     * @brief Builds the common entity state.
     * @param number Protocol number within the entity type.
     * @param position Initial grid position.
     * @param orientation Initial facing orientation.
     */
    AMapEntity(int number, GridPosition position, Orientation orientation);

    ~AMapEntity() override = default;

    /** @brief Protocol number within the entity type. @return The number. */
    int number() const override;

    /** @brief Current grid position. @return The coordinates. */
    GridPosition position() const override;

    /** @brief Current facing orientation. @return The orientation. */
    Orientation orientation() const override;

    /** @brief Updates the grid position. @param position New grid position. */
    void setPosition(GridPosition position) override;

    /** @brief Updates the facing orientation. @param orientation New orientation. */
    void setOrientation(Orientation orientation) override;

    /**
     * @brief Base info lines shared by every entity: identity and position.
     * @return The identity and position lines (concrete types append their own).
     */
    std::vector<std::string> infoLines() const override;

  protected:
    int _number;              ///< Protocol number within the entity type.
    GridPosition _position;   ///< Current grid position.
    Orientation _orientation; ///< Current facing orientation.
};

} // namespace Zappy
