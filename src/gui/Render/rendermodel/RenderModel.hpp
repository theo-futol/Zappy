#pragma once

#include <cstddef>
#include <exception>
#include <string>
#include <vector>

#include "Graphics/mesh/Mesh.hpp"
#include "Graphics/shader/Shader.hpp"
#include "Render/asset/AssetCache.hpp"
#include "types/Mat.hpp"
#include "types/MeshData.hpp"
#include "types/Model.hpp"
#include "types/Vec.hpp"

namespace Zappy
{

/**
 * @class RenderModel
 * @brief A decoded Model uploaded to the GPU and ready to draw.
 *
 * Built from a CPU-side Model and an AssetCache: it interleaves each primitive
 * into a {3, 3, 2} mesh, flattens the node hierarchy into a list of draw items
 * (each with its world transform), and computes a bounding sphere for framing.
 */
class RenderModel
{
  public:
    /**
     * @class RenderModelException
     * @brief Error raised while uploading a model to the GPU.
     */
    class RenderModelException : public std::exception
    {
      public:
        explicit RenderModelException(const std::string &message) : _message(message)
        {
        }

        const char *what() const noexcept override
        {
            return _message.c_str();
        }

      private:
        std::string _message; ///< Error description.
    };

    /**
     * @brief Uploads a model's geometry to the GPU and flattens its node tree.
     * @param model Decoded CPU-side model.
     * @param assets Asset cache that will own the created meshes and textures.
     * @param id Unique id prefix used to key the resources in the cache.
     * @param directory Directory of the model file, used to resolve relative texture paths.
     * @throws RenderModelException On a mesh or texture upload failure.
     */
    RenderModel(const Model &model, AssetCache &assets, const std::string &id, const std::string &directory);

    /**
     * @brief Draws the model once per base transform, in one instanced call per part.
     *
     * For every part (primitive) it builds the per-instance model matrices
     * (each base composed with the part's node transform) and issues a single
     * instanced draw. All bases must share the same scale/rotation so one shared
     * normal matrix is correct; only their translation may differ.
     * @param shader Phong shader, already bound and given the frame uniforms.
     * @param bases One base transform per instance (e.g. one per tile or per entity).
     */
    void drawInstanced(Shader &shader, const std::vector<Mat4> &bases) const;

    /** @brief Center of the normalized bounding sphere. @return The center. */
    Vec3 center() const;

    /** @brief Radius of the normalized bounding sphere. @return The radius. */
    float radius() const;

    /**
     * @brief Transform that fits the raw model into a unit footprint standing on the ground.
     *
     * Centers the model horizontally on the origin, drops its base to y = 0, and scales
     * it so its largest extent equals one tile. Compose it with a tile world position to
     * place the model on that tile.
     * @return The normalization transform.
     */
    Mat4 unitTransform() const;

    /**
     * @brief Transform that fits the model's horizontal footprint to one tile, lying flat.
     *
     * Centers the model on the origin, scales its X and Z extents to one tile each
     * (proportional height), and puts its top at y = 0. Suited to flat ground tiles
     * that other models stand on. Compose it with a tile world position to tile the ground.
     * @return The footprint-fit transform.
     */
    Mat4 footprintTransform() const;

  private:
    /**
     * @struct DrawItem
     * @brief One mesh to draw with its node world transform.
     */
    struct DrawItem
    {
        Mesh *mesh;              ///< Mesh to draw (owned by the AssetCache).
        const Texture *texture;  ///< Base-color texture, or nullptr if untextured.
        Mat4 transform;          ///< World transform of the node referencing it.
    };

    /**
     * @brief Interleaves a primitive's attributes into a {3, 3, 2} vertex buffer.
     * @param primitive Source primitive (de-interleaved arrays).
     * @return The interleaved mesh data.
     */
    static MeshData interleave(const ModelPrimitive &primitive);

    /**
     * @brief Uploads every primitive as a mesh and records its local bounds.
     * @param model Source model.
     * @param assets Asset cache receiving the meshes.
     * @param id Mesh id prefix.
     * @throws RenderModelException On a mesh upload failure.
     */
    void uploadPrimitives(const Model &model, AssetCache &assets, const std::string &id);

    /**
     * @brief Loads the model's textures, resolving their paths relative to the model directory.
     * @param model Source model.
     * @param assets Asset cache receiving the textures.
     * @param id Texture id prefix.
     * @param directory Directory of the model file.
     * @throws RenderModelException On a texture load failure.
     */
    void loadTextures(const Model &model, AssetCache &assets, const std::string &id, const std::string &directory);

    /**
     * @brief Recursively flattens a node and its children into draw items, expanding the bounds.
     * @param model Source model.
     * @param nodeIndex Index of the node to visit.
     * @param parent Accumulated world transform of the parent.
     * @param boundsMin Running minimum corner of the world-space bounding box.
     * @param boundsMax Running maximum corner of the world-space bounding box.
     */
    void flattenNode(const Model &model, std::size_t nodeIndex, const Mat4 &parent, Vec3 &boundsMin, Vec3 &boundsMax);

    std::vector<Mesh *> _meshes;           ///< One mesh per source primitive (index = primitive index).
    std::vector<const Texture *> _textures; ///< One texture per Model::textures entry.
    std::vector<Vec3> _localMin;           ///< Per-primitive local AABB minimum.
    std::vector<Vec3> _localMax;  ///< Per-primitive local AABB maximum.
    std::vector<DrawItem> _items;      ///< Flattened draw list.
    Mat4 _unitTransform;               ///< Raw-to-unit normalization transform (stand on ground).
    Mat4 _footprintTransform;          ///< Raw-to-tile normalization transform (flat, fills one tile).
    Vec3 _center;                      ///< Normalized bounding-sphere center.
    float _radius;                ///< Normalized bounding-sphere radius.

    static constexpr float TargetSize = 1.0f; ///< Largest extent of a normalized model (in tiles).
};

} // namespace Zappy
