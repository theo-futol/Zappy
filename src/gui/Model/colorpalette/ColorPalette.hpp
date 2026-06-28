#pragma once

#include <cstddef>
#include <vector>

#include "types/Color.hpp"

namespace Zappy
{

/**
 * @class ColorPalette
 * @brief Hands out visually distinct colors, cycling through a fixed palette.
 */
class ColorPalette
{
  public:
    ColorPalette();

    /**
     * @brief Returns the next distinct color, wrapping when the palette is exhausted.
     * @return A palette color.
     */
    Color next();

  private:
    std::vector<Color> _colors; ///< Available colors.
    std::size_t _index;         ///< Index of the next color to hand out.
};

} // namespace Zappy
