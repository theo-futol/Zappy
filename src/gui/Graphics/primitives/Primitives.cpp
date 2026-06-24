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

} // namespace Zappy
