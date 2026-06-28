#pragma once

namespace Zappy
{

/**
 * @enum RenderMode
 * @brief Display mode chosen at startup via the -m flag.
 *
 * Selected once in Core and passed to RenderSystem, which wires the matching
 * camera, projection and renderers. The modes share the same window,
 * context and game state; only the view pipeline differs.
 */
enum class RenderMode
{
    TwoD,       ///< Flat top-down view (default, -m 2d).
    ThreeD,     ///< 3D view on a flat ground (-m 3d).
    ThreeDTorus ///< 3D view wrapping the map onto a torus planet (-m torus).
};

} // namespace Zappy
