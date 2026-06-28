#include "Graphics/primitives/Primitives.hpp"

#include <cmath>

namespace Zappy
{

namespace
{
constexpr float Tau = 6.28318530718f; ///< 2*pi.
} // namespace

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

MeshData Primitives::groundRing(std::size_t segments, float thickness)
{
    MeshData data;
    float outer = 0.5f;
    float inner = outer * (1.0f - thickness);

    for (std::size_t i = 0; i <= segments; ++i)
    {
        float angle = Tau * static_cast<float>(i) / static_cast<float>(segments);
        float c = std::cos(angle);
        float s = std::sin(angle);

        data.vertices.insert(data.vertices.end(), {inner * c, 0.0f, inner * s, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f});
        data.vertices.insert(data.vertices.end(), {outer * c, 0.0f, outer * s, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f});
    }
    for (unsigned int i = 0; i < static_cast<unsigned int>(segments); ++i)
    {
        unsigned int a = i * 2;

        data.indices.insert(data.indices.end(), {a, a + 1, a + 3, a, a + 3, a + 2});
    }
    return data;
}

MeshData Primitives::ring(std::size_t segments, float thickness)
{
    MeshData data;
    float outer = 0.5f;
    float inner = outer * (1.0f - thickness);

    for (std::size_t i = 0; i <= segments; ++i)
    {
        float angle = Tau * static_cast<float>(i) / static_cast<float>(segments);
        float c = std::cos(angle);
        float s = std::sin(angle);

        data.vertices.insert(data.vertices.end(), {inner * c, inner * s, 0.0f});
        data.vertices.insert(data.vertices.end(), {outer * c, outer * s, 0.0f});
    }
    for (unsigned int i = 0; i < static_cast<unsigned int>(segments); ++i)
    {
        unsigned int a = i * 2;

        data.indices.insert(data.indices.end(), {a, a + 1, a + 3, a, a + 3, a + 2});
    }
    return data;
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
