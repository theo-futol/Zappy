#pragma once

#include <string>

#include "abstract/AMapEntity.hpp"
#include "types/Appearance.hpp"
#include "types/Color.hpp"
#include "types/GridPosition.hpp"

namespace Zappy
{

/**
 * @class Egg
 * @brief An egg laid on the map, identified and with its own lifecycle.
 */
class Egg : public AMapEntity
{
  public:
    /**
     * @brief Builds an egg.
     * @param number Egg number.
     * @param position Grid position.
     * @param team Team the egg belongs to.
     * @param color Team display color.
     */
    Egg(int number, GridPosition position, const std::string &team, Color color);

    /** @brief Entity type. @return The string "egg". */
    std::string getEntityType() const override;

    /** @brief Visual description declared by the egg. @return The appearance. */
    Appearance appearance() const override;

    /** @brief Team name. @return The team. */
    const std::string &team() const;

  private:
    std::string _team; ///< Owning team name.
    Color _color;      ///< Team display color.
};

} // namespace Zappy
