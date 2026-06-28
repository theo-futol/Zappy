#include "Graphics/primitives/Primitives.hpp"

namespace Zappy
{

MeshData Primitives::quad()
{
    return MeshData{
        {
            -0.5f, -0.5f, 0.0f, // bottom-left
            0.5f, -0.5f, 0.0f,  // bottom-right
            0.5f, 0.5f, 0.0f,   // top-right
            -0.5f, 0.5f, 0.0f,  // top-left
        },
        {0, 1, 2, 2, 3, 0},
    };
}

MeshData Primitives::groundQuad()
{
    return MeshData{
        {
            -0.5f, 0.0f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, // back-left
            0.5f,  0.0f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // back-right
            0.5f,  0.0f, 0.5f,  0.0f, 1.0f, 0.0f, 1.0f, 1.0f, // front-right
            -0.5f, 0.0f, 0.5f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // front-left
        },
        {0, 1, 2, 2, 3, 0},
    };
}

MeshData Primitives::triangle()
{
    return MeshData{
        {
            0.0f, 0.5f, 0.0f,   // tip, toward +Y
            -0.5f, -0.5f, 0.0f, // bottom-left
            0.5f, -0.5f, 0.0f,  // bottom-right
        },
        {0, 1, 2},
    };
}

MeshData Primitives::cube()
{
    return MeshData{
        {
            // Each row: position(x,y,z) | normal(x,y,z) | uv(u,v). 4 vertices per face, CCW seen from outside.
            // Front face (+Z)
            -0.5f,
            -0.5f,
            0.5f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            0.5f,
            -0.5f,
            0.5f,
            0.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            0.5f,
            0.5f,
            0.5f,
            0.0f,
            0.0f,
            1.0f,
            1.0f,
            1.0f,
            -0.5f,
            0.5f,
            0.5f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            1.0f,
            // Back face (-Z)
            0.5f,
            -0.5f,
            -0.5f,
            0.0f,
            0.0f,
            -1.0f,
            0.0f,
            0.0f,
            -0.5f,
            -0.5f,
            -0.5f,
            0.0f,
            0.0f,
            -1.0f,
            1.0f,
            0.0f,
            -0.5f,
            0.5f,
            -0.5f,
            0.0f,
            0.0f,
            -1.0f,
            1.0f,
            1.0f,
            0.5f,
            0.5f,
            -0.5f,
            0.0f,
            0.0f,
            -1.0f,
            0.0f,
            1.0f,
            // Left face (-X)
            -0.5f,
            -0.5f,
            -0.5f,
            -1.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            -0.5f,
            -0.5f,
            0.5f,
            -1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            -0.5f,
            0.5f,
            0.5f,
            -1.0f,
            0.0f,
            0.0f,
            1.0f,
            1.0f,
            -0.5f,
            0.5f,
            -0.5f,
            -1.0f,
            0.0f,
            0.0f,
            0.0f,
            1.0f,
            // Right face (+X)
            0.5f,
            -0.5f,
            0.5f,
            1.0f,
            0.0f,
            0.0f,
            0.0f,
            0.0f,
            0.5f,
            -0.5f,
            -0.5f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            0.0f,
            0.5f,
            0.5f,
            -0.5f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            1.0f,
            0.5f,
            0.5f,
            0.5f,
            1.0f,
            0.0f,
            0.0f,
            0.0f,
            1.0f,
            // Top face (+Y)
            -0.5f,
            0.5f,
            0.5f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            0.0f,
            0.5f,
            0.5f,
            0.5f,
            0.0f,
            1.0f,
            0.0f,
            1.0f,
            0.0f,
            0.5f,
            0.5f,
            -0.5f,
            0.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            -0.5f,
            0.5f,
            -0.5f,
            0.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            // Bottom face (-Y)
            -0.5f,
            -0.5f,
            -0.5f,
            0.0f,
            -1.0f,
            0.0f,
            0.0f,
            0.0f,
            0.5f,
            -0.5f,
            -0.5f,
            0.0f,
            -1.0f,
            0.0f,
            1.0f,
            0.0f,
            0.5f,
            -0.5f,
            0.5f,
            0.0f,
            -1.0f,
            0.0f,
            1.0f,
            1.0f,
            -0.5f,
            -0.5f,
            0.5f,
            0.0f,
            -1.0f,
            0.0f,
            0.0f,
            1.0f,
        },
        {
            0,  1,  2,  2,  3,  0,  // front
            4,  5,  6,  6,  7,  4,  // back
            8,  9,  10, 10, 11, 8,  // left
            12, 13, 14, 14, 15, 12, // right
            16, 17, 18, 18, 19, 16, // top
            20, 21, 22, 22, 23, 20, // bottom
        },
    };
}

} // namespace Zappy
