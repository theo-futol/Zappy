#pragma once

#include <string>
#include <vector>

#include "Model/resourceset/ResourceSet.hpp"
#include "abstract/AMapEntity.hpp"
#include "types/Appearance.hpp"
#include "types/Color.hpp"
#include "types/GridPosition.hpp"
#include "types/Orientation.hpp"

namespace Zappy
{

/**
 * @class Trantorian
 * @brief A Trantorian inhabitant, the protocol's "player".
 */
class Trantorian : public AMapEntity
{
  public:
    /**
     * @brief Builds a Trantorian.
     * @param number Player number.
     * @param position Initial grid position.
     * @param orientation Initial orientation.
     * @param team Team name.
     * @param level Player level.
     * @param color Team display color.
     */
    Trantorian(int number, GridPosition position, Orientation orientation, const std::string &team, int level, Color color);

    /** @brief Entity type. @return The string "player". */
    std::string getEntityType() const override;

    /** @brief Visual description declared by the Trantorian. @return The appearance. */
    Appearance appearance() const override;

    /** @brief Info lines: identity, position, team, level and inventory. @return The lines. */
    std::vector<std::string> infoLines() const override;

    /** @brief Team name. @return The team. */
    const std::string &team() const override;

    /** @brief Player level. @return The level. */
    int level() const;

    /** @brief Sets the player level. @param level New level. */
    void setLevel(int level);

    /** @brief Mutable carried inventory. @return The resource set. */
    ResourceSet &inventory();

    /** @brief Read-only carried inventory. @return The resource set. */
    const ResourceSet &inventory() const;

  private:
    std::string _team;      ///< Team name.
    int _level;             ///< Player level.
    ResourceSet _inventory; ///< Carried resources.
    Color _color;           ///< Team display color.
};

} // namespace Zappy
