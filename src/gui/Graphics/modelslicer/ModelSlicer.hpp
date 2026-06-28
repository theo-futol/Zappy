#pragma once

#include <cstddef>
#include <string>

#include "types/Mat.hpp"
#include "types/Model.hpp"

namespace Zappy
{

/**
 * @class ModelSlicer
 * @brief Extracts a single sub-object out of a multi-object Model, keyed by texture.
 *
 * A decorative scene (e.g. a pile of crystals and rocks) packs many distinct
 * objects into one Model. This slicer rebuilds a standalone Model holding only
 * the primitives whose texture matches a given name, with their geometry baked
 * into world space so the result is self-contained (a single identity root node)
 * and can be normalized and placed on its own, like any other model.
 */
class ModelSlicer
{
  public:
    /**
     * @brief Builds a standalone Model from the primitives using a matching texture.
     * @param full Source multi-object model.
     * @param textureNeedle Substring matched against each texture URI (e.g. "rock2_baseColor").
     * @return A self-contained model with the matching, world-baked geometry and that one texture.
     */
    Model byTexture(const Model &full, const std::string &textureNeedle) const;

  private:
    /**
     * @brief Walks the node tree, baking matching primitives into the output model.
     * @param full Source model.
     * @param nodeIndex Index of the node to visit.
     * @param parent Accumulated world transform of the parent.
     * @param needle Texture substring selecting the primitives to keep.
     * @param out Output model being filled (its node 0 receives the kept primitives).
     */
    void collect(const Model &full, std::size_t nodeIndex, const Mat4 &parent, const std::string &needle, Model &out) const;

    /**
     * @brief Copies a primitive with its positions and normals transformed to world space.
     * @param source Source primitive (in its local space).
     * @param world World transform to bake into the positions.
     * @param normalMatrix Matching normal matrix (transpose-inverse of the world's upper-left 3x3).
     * @return The world-baked primitive, textured by index 0.
     */
    ModelPrimitive bake(const ModelPrimitive &source, const Mat4 &world, const Mat3 &normalMatrix) const;
};

} // namespace Zappy
