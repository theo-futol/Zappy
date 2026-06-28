#pragma once

#include <string>

#include "types/Color.hpp"

namespace Zappy
{

/**
 * @struct Appearance
 * @brief View-independent visual description an entity declares about itself.
 */
struct Appearance
{
    std::string visualId; ///< Symbolic asset identifier resolved by the AssetCache.
    Color color;          ///< Tint applied to the visual (e.g. team color).
    float scale;          ///< Relative size factor.
};

} // namespace Zappy
