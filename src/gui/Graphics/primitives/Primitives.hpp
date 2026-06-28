#pragma once

#include "types/MeshData.hpp"

namespace Zappy
{

/**
 * @class Primitives
 * @brief Source of reusable primitive-shape geometry. Adding a shape is one static method.
 */
class Primitives
{
  public:
    /**
     * @brief Unit quad centered on the origin, lying on the XY plane.
     * @return Its vertex and index data.
     */
    static MeshData quad();

    /**
     * @brief Unit quad centered on the origin, lying flat on the XZ plane (normal up).
     *
     * Interleaved as position (3) + normal (3) + uv (2): layout {3, 3, 2}. Suited to
     * a ground marker drawn just above a tile.
     * @return Its vertex and index data.
     */
    static MeshData groundQuad();

    /**
     * @brief Triangle pointing toward +Y, centered on the origin.
     * @return Its vertex and index data.
     */
    static MeshData triangle();

    /**
     * @brief Unit cube centered on the origin (side 1), with per-face normals and UVs.
     *
     * Interleaved as position (3) + normal (3) + uv (2): layout {3, 3, 2}. Each of
     * the 6 faces has its own 4 vertices (24 total) so normals and UVs are not
     * shared across faces; 36 indices (2 triangles per face).
     * @return Its vertex and index data.
     */
    static MeshData cube();
};

} // namespace Zappy
