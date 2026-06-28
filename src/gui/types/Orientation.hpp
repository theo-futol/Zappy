#pragma once

namespace Zappy
{

/**
 * @enum Orientation
 * @brief Cardinal orientation of an entity, matching the GUI protocol field O.
 */
enum class Orientation
{
    North = 1, ///< Facing north (protocol value 1).
    East = 2,  ///< Facing east (protocol value 2).
    South = 3, ///< Facing south (protocol value 3).
    West = 4   ///< Facing west (protocol value 4).
};

} // namespace Zappy
