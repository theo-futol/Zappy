#pragma once

#include <exception>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "Graphics/mesh/Mesh.hpp"
#include "Graphics/shader/Shader.hpp"
#include "Graphics/texture/Texture.hpp"

namespace Zappy
{

/**
 * @class AssetCache
 * @brief Owns the shared GPU resources, keyed by symbolic id, loaded once.
 *
 * Entities reference a visual by id (their Appearance); the renderer resolves it
 * here, so resources are never duplicated per entity.
 */
class AssetCache
{
  public:
    /**
     * @class AssetCacheException
     * @brief Error raised while loading an asset.
     */
    class AssetCacheException : public std::exception
    {
      public:
        explicit AssetCacheException(const std::string &message) : _message(message)
        {
        }

        const char *what() const noexcept override
        {
            return _message.c_str();
        }

      private:
        std::string _message; ///< Error description.
    };

    AssetCache();

    /**
     * @brief Compiles a shader program from source files and caches it under an id.
     * @param id Symbolic shader identifier.
     * @param vertexPath Vertex shader source path.
     * @param fragmentPath Fragment shader source path.
     * @return Reference to the cached shader.
     * @throws AssetCacheException On a shader compilation or linking error.
     */
    Shader &loadShader(const std::string &id, const std::string &vertexPath, const std::string &fragmentPath);

    /**
     * @brief Loads an image into a texture and caches it under an id.
     * @param id Symbolic texture identifier.
     * @param path Path to the image file.
     * @return Reference to the cached texture.
     * @throws AssetCacheException On an image load or decode error.
     */
    Texture &loadTexture(const std::string &id, const std::string &path);

    /**
     * @brief Creates and uploads a mesh, then caches it under an id.
     * @param id Symbolic mesh identifier.
     * @param vertices Interleaved vertex attributes.
     * @param indices Element indices.
     * @param attributeSizes Component count of each attribute, in layout order (see Mesh::upload).
     * @return Reference to the cached mesh.
     * @throws AssetCacheException On a mesh upload error.
     */
    Mesh &createMesh(const std::string &id, const std::vector<float> &vertices, const std::vector<unsigned int> &indices, const std::vector<unsigned int> &attributeSizes);

    /**
     * @brief Looks up a texture by id.
     * @param id Symbolic texture identifier.
     * @return Pointer to the texture, or nullptr if not loaded.
     */
    Texture *texture(const std::string &id);

    /**
     * @brief Looks up a mesh by id.
     * @param id Symbolic mesh identifier.
     * @return Pointer to the mesh, or nullptr if not loaded.
     */
    Mesh *mesh(const std::string &id);

    /**
     * @brief Looks up a shader by id.
     * @param id Symbolic shader identifier.
     * @return Pointer to the shader, or nullptr if not loaded.
     */
    Shader *shader(const std::string &id);

  private:
    std::map<std::string, std::unique_ptr<Texture>> _textures; ///< Cached textures.
    std::map<std::string, std::unique_ptr<Mesh>> _meshes;      ///< Cached meshes.
    std::map<std::string, std::unique_ptr<Shader>> _shaders;   ///< Cached shaders.
};

} // namespace Zappy
