#include "Render/asset/AssetCache.hpp"

namespace Zappy
{

AssetCache::AssetCache() : _textures(), _meshes(), _shaders()
{
}

Shader &AssetCache::loadShader(const std::string &id, const std::string &vertexPath, const std::string &fragmentPath)
{
    try
    {
        std::unique_ptr<Shader> shader = std::make_unique<Shader>(vertexPath, fragmentPath);
        Shader &ref = *shader;

        _shaders[id] = std::move(shader);
        return ref;
    }
    catch (const Shader::ShaderException &e)
    {
        throw AssetCacheException("shader '" + id + "': " + std::string(e.what()));
    }
}

Mesh &AssetCache::createMesh(const std::string &id, const std::vector<float> &vertices, const std::vector<unsigned int> &indices)
{
    try
    {
        std::unique_ptr<Mesh> mesh = std::make_unique<Mesh>();

        mesh->upload(vertices, indices);
        Mesh &ref = *mesh;
        _meshes[id] = std::move(mesh);
        return ref;
    }
    catch (const Mesh::MeshException &e)
    {
        throw AssetCacheException("mesh '" + id + "': " + std::string(e.what()));
    }
}

Texture *AssetCache::texture(const std::string &id)
{
    auto it = _textures.find(id);

    if (it == _textures.end())
        return nullptr;
    return it->second.get();
}

Mesh *AssetCache::mesh(const std::string &id)
{
    auto it = _meshes.find(id);

    if (it == _meshes.end())
        return nullptr;
    return it->second.get();
}

Shader *AssetCache::shader(const std::string &id)
{
    auto it = _shaders.find(id);

    if (it == _shaders.end())
        return nullptr;
    return it->second.get();
}

} // namespace Zappy
