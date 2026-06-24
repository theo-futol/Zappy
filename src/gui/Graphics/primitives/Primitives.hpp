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
     * @brief Triangle pointing toward +Y, centered on the origin.
     * @return Its vertex and index data.
     */
    static MeshData triangle();
};

} // namespace Zappy
