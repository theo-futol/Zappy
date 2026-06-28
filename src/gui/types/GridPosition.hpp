#pragma once

namespace Zappy
{

/**
 * @struct GridPosition
 * @brief Integer coordinates on the toroidal game grid.
 */
struct GridPosition
{
    int x; ///< Column (wraps horizontally).
    int y; ///< Row (wraps vertically).
};

} // namespace Zappy
