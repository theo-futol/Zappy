#pragma once

#include <cstddef>
#include <vector>

#include "types/Theme.hpp"

namespace Zappy
{

/**
 * @class ThemeRegistry
 * @brief Assigns each team a paired golem+egg model, by arrival order, cycling past the last theme.
 *
 * Teams are registered in the order the server announces them; this registry maps
 * the n-th team to the n-th theme. When there are more teams than themes, the
 * assignment wraps around (team 5 reuses theme 1, etc.).
 */
class ThemeRegistry
{
  public:
    ThemeRegistry();

    /**
     * @brief Theme assigned to a team by its arrival index, wrapping when exhausted.
     * @param teamIndex 0-based index of the team in arrival order.
     * @return The team's theme.
     */
    const Theme &forTeam(std::size_t teamIndex) const;

    /** @brief Number of distinct themes available. @return The theme count. */
    std::size_t count() const;

  private:
    std::vector<Theme> _themes; ///< Distinct team themes, in assignment order.
};

} // namespace Zappy
