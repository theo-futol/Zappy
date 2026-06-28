#pragma once

#include <vector>

namespace Zappy
{

/**
 * @struct MeshData
 * @brief Raw geometry (vertices + indices) for uploading a Mesh, independent of OpenGL.
 */
struct MeshData
{
    std::vector<float> vertices;       ///< Interleaved vertex attributes.
    std::vector<unsigned int> indices; ///< Element indices.
};

} // namespace Zappy
