#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "types/Mat.hpp"

namespace Zappy
{

/**
 * @struct ModelPrimitive
 * @brief One drawable chunk of a model: CPU-side geometry plus its texture reference.
 *
 * Attributes are stored de-interleaved (one vector per channel) as glTF delivers
 * them; the GPU-upload layer is free to interleave them. `normals`/`texcoords`
 * are empty when the source primitive lacks that attribute.
 */
struct ModelPrimitive
{
    std::vector<float> positions;      ///< Vertex positions, 3 floats per vertex (xyz).
    std::vector<float> normals;        ///< Vertex normals, 3 floats per vertex (empty if absent).
    std::vector<float> texcoords;      ///< Texture coords, 2 floats per vertex (empty if absent).
    std::vector<unsigned int> indices; ///< Triangle indices into the vertex arrays.
    int texture;                       ///< Index into Model::textures, or -1 when untextured.
};

/**
 * @struct ModelNode
 * @brief A node of the model's scene graph: a local transform and its children.
 *
 * `name` is preserved so animation code can address a node (e.g. a limb) by name.
 * Indices reference the flat vectors held by the owning Model.
 */
struct ModelNode
{
    std::string name;                  ///< Node name from the glTF file (may be empty).
    Mat4 transform;                    ///< Local transform of this node (TRS or matrix).
    std::vector<std::size_t> primitives; ///< Indices into Model::primitives drawn at this node.
    std::vector<std::size_t> children;   ///< Indices into Model::nodes parented to this node.
};

/**
 * @struct Model
 * @brief A fully decoded 3D model: geometry, textures and scene hierarchy.
 *
 * Pure CPU-side data produced by ModelLoader; it carries no OpenGL resource and
 * no logic. GPU upload and rendering are the responsibility of other classes.
 */
struct Model
{
    std::vector<ModelPrimitive> primitives; ///< All primitives, referenced by nodes.
    std::vector<std::string> textures;      ///< Texture image URIs, relative to the model file.
    std::vector<ModelNode> nodes;           ///< All scene-graph nodes.
    std::vector<std::size_t> roots;         ///< Indices of top-level nodes in `nodes`.
};

} // namespace Zappy
