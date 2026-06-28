#pragma once

#include <cstddef>
#include <exception>
#include <string>
#include <utility>
#include <vector>

#include <lib/cgltf/cgltf.h>

#include "types/Model.hpp"

namespace Zappy
{

/**
 * @class ModelLoader
 * @brief Loads a glTF/glb 3D model into CPU-side Model data.
 *
 * Sole owner of the vendored cgltf library (system-primitive-style confinement,
 * like Texture for stb_image): no other class calls a cgltf function. It parses
 * the file, loads its buffers and decodes geometry, materials and the scene
 * graph. It does not touch OpenGL and does not render — GPU upload and drawing
 * belong to other classes.
 */
class ModelLoader
{
  public:
    /**
     * @class ModelLoaderException
     * @brief Error raised while parsing or decoding a glTF model.
     */
    class ModelLoaderException : public std::exception
    {
      public:
        explicit ModelLoaderException(const std::string &message) : _message(message)
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
     * @brief Parses a glTF/glb file and decodes it into a Model.
     * @param path Path to the .gltf or .glb file.
     * @return The decoded model (geometry, textures, scene graph).
     * @throws ModelLoaderException On parse, buffer-load or validation failure.
     */
    Model load(const std::string &path) const;

  private:
    /**
     * @brief Builds the flat primitive list and a mesh->range map.
     * @param data Parsed cgltf document.
     * @param model Model whose `primitives`/`textures` are filled.
     * @return For each mesh index, the [first, count) range of its primitives.
     */
    std::vector<std::pair<std::size_t, std::size_t>> readMeshes(const cgltf_data &data, Model &model) const;

    /**
     * @brief Decodes a single primitive's attributes, indices and texture.
     * @param primitive Source cgltf primitive.
     * @param model Model whose `textures` list is appended to when resolving the texture.
     * @return The decoded primitive.
     */
    ModelPrimitive readPrimitive(const cgltf_primitive &primitive, Model &model) const;

    /**
     * @brief Reads a float attribute (3 or 2 components) of a primitive.
     * @param primitive Source primitive.
     * @param type Attribute type to read (position, normal, texcoord).
     * @param components Number of components per element (3 or 2).
     * @return The packed values, or an empty vector if the attribute is absent.
     */
    std::vector<float> readAttribute(const cgltf_primitive &primitive, cgltf_attribute_type type, std::size_t components) const;

    /**
     * @brief Resolves a primitive's base-color texture to an index into Model::textures.
     * @param primitive Source primitive.
     * @param model Model whose `textures` list is appended to on first sight.
     * @return The texture index, or -1 when the primitive is untextured.
     */
    int resolveTexture(const cgltf_primitive &primitive, Model &model) const;

    /**
     * @brief Builds the scene-graph nodes and the root list.
     * @param data Parsed cgltf document.
     * @param meshRanges Per-mesh primitive ranges from readMeshes.
     * @param model Model whose `nodes`/`roots` are filled.
     */
    void readNodes(const cgltf_data &data, const std::vector<std::pair<std::size_t, std::size_t>> &meshRanges, Model &model) const;

    static constexpr std::size_t Vec3Size = 3; ///< Components of a position/normal element.
    static constexpr std::size_t Vec2Size = 2; ///< Components of a texcoord element.
};

} // namespace Zappy
