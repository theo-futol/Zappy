#pragma once

#include <string>

namespace Zappy
{

/**
 * @struct Theme
 * @brief Paired 3D models identifying one team visually: its golem and its matching egg.
 */
struct Theme
{
    std::string golem; ///< Path to the player/golem model (.gltf).
    std::string egg;   ///< Path to the matching egg model (.gltf).
};

} // namespace Zappy
