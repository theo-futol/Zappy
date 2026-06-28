#include "Render/rendermodel/RenderModel.hpp"

#include <algorithm>
#include <limits>

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

namespace Zappy
{

RenderModel::RenderModel(const Model &model, AssetCache &assets, const std::string &id, const std::string &directory)
    : _meshes(), _textures(), _localMin(), _localMax(), _items(), _unitTransform(1.0f), _footprintTransform(1.0f), _center(0.0f), _radius(1.0f)
{
    Vec3 boundsMin(std::numeric_limits<float>::max());
    Vec3 boundsMax(std::numeric_limits<float>::lowest());

    uploadPrimitives(model, assets, id);
    loadTextures(model, assets, id, directory);
    for (std::size_t root : model.roots)
        flattenNode(model, root, Mat4(1.0f), boundsMin, boundsMax);
    if (_items.empty())
        return;

    Vec3 rawCenter = (boundsMin + boundsMax) * 0.5f;
    Vec3 extent = boundsMax - boundsMin;
    float maxExtent = std::max(extent.x, std::max(extent.y, extent.z));
    float scale = (maxExtent > 0.0f) ? TargetSize / maxExtent : 1.0f;
    Mat4 toPivot = glm::translate(Mat4(1.0f), -Vec3(rawCenter.x, boundsMin.y, rawCenter.z));

    _unitTransform = glm::scale(Mat4(1.0f), Vec3(scale)) * toPivot;
    _center = Vec3(0.0f, extent.y * scale * 0.5f, 0.0f);
    _radius = glm::length(extent) * scale * 0.5f;

    float footX = (extent.x > 0.0f) ? 1.0f / extent.x : 1.0f;
    float footZ = (extent.z > 0.0f) ? 1.0f / extent.z : 1.0f;
    float footY = std::min(footX, footZ);
    Mat4 toTop = glm::translate(Mat4(1.0f), -Vec3(rawCenter.x, boundsMax.y, rawCenter.z));

    _footprintTransform = glm::scale(Mat4(1.0f), Vec3(footX, footY, footZ)) * toTop;
}

MeshData RenderModel::interleave(const ModelPrimitive &primitive)
{
    MeshData data;
    std::size_t count = primitive.positions.size() / 3;

    data.vertices.reserve(count * 8);
    for (std::size_t i = 0; i < count; ++i)
    {
        data.vertices.push_back(primitive.positions[i * 3 + 0]);
        data.vertices.push_back(primitive.positions[i * 3 + 1]);
        data.vertices.push_back(primitive.positions[i * 3 + 2]);
        if (primitive.normals.size() >= (i + 1) * 3)
        {
            data.vertices.push_back(primitive.normals[i * 3 + 0]);
            data.vertices.push_back(primitive.normals[i * 3 + 1]);
            data.vertices.push_back(primitive.normals[i * 3 + 2]);
        }
        else
        {
            data.vertices.push_back(0.0f);
            data.vertices.push_back(1.0f);
            data.vertices.push_back(0.0f);
        }
        if (primitive.texcoords.size() >= (i + 1) * 2)
        {
            data.vertices.push_back(primitive.texcoords[i * 2 + 0]);
            data.vertices.push_back(primitive.texcoords[i * 2 + 1]);
        }
        else
        {
            data.vertices.push_back(0.0f);
            data.vertices.push_back(0.0f);
        }
    }
    data.indices = primitive.indices;
    return data;
}

void RenderModel::uploadPrimitives(const Model &model, AssetCache &assets, const std::string &id)
{
    for (std::size_t i = 0; i < model.primitives.size(); ++i)
    {
        const ModelPrimitive &primitive = model.primitives[i];
        MeshData data = interleave(primitive);
        Vec3 localMin(std::numeric_limits<float>::max());
        Vec3 localMax(std::numeric_limits<float>::lowest());

        for (std::size_t v = 0; v + 2 < primitive.positions.size(); v += 3)
        {
            Vec3 point(primitive.positions[v], primitive.positions[v + 1], primitive.positions[v + 2]);

            localMin = glm::min(localMin, point);
            localMax = glm::max(localMax, point);
        }
        try
        {
            Mesh &mesh = assets.createMesh(id + "#" + std::to_string(i), data.vertices, data.indices, {3, 3, 2});

            _meshes.push_back(&mesh);
        }
        catch (const AssetCache::AssetCacheException &e)
        {
            throw RenderModelException(std::string("RenderModel error: ") + e.what());
        }
        _localMin.push_back(localMin);
        _localMax.push_back(localMax);
    }
}

void RenderModel::loadTextures(const Model &model, AssetCache &assets, const std::string &id, const std::string &directory)
{
    for (std::size_t i = 0; i < model.textures.size(); ++i)
    {
        try
        {
            Texture &texture = assets.loadTexture(id + "_tex#" + std::to_string(i), directory + "/" + model.textures[i]);

            _textures.push_back(&texture);
        }
        catch (const AssetCache::AssetCacheException &e)
        {
            throw RenderModelException(std::string("RenderModel error: ") + e.what());
        }
    }
}

void RenderModel::flattenNode(const Model &model, std::size_t nodeIndex, const Mat4 &parent, Vec3 &boundsMin, Vec3 &boundsMax)
{
    const ModelNode &node = model.nodes[nodeIndex];
    Mat4 world = parent * node.transform;

    for (std::size_t primitive : node.primitives)
    {
        int textureIndex = model.primitives[primitive].texture;
        const Texture *texture = (textureIndex >= 0 && static_cast<std::size_t>(textureIndex) < _textures.size()) ? _textures[textureIndex] : nullptr;

        _items.push_back(DrawItem{_meshes[primitive], texture, world});
        for (int corner = 0; corner < 8; ++corner)
        {
            Vec3 lo = _localMin[primitive];
            Vec3 hi = _localMax[primitive];
            Vec3 point((corner & 1) ? hi.x : lo.x, (corner & 2) ? hi.y : lo.y, (corner & 4) ? hi.z : lo.z);
            Vec3 worldPoint(world * Vec4(point, 1.0f));

            boundsMin = glm::min(boundsMin, worldPoint);
            boundsMax = glm::max(boundsMax, worldPoint);
        }
    }
    for (std::size_t child : node.children)
        flattenNode(model, child, world, boundsMin, boundsMax);
}

void RenderModel::drawInstanced(Shader &shader, const std::vector<Mat4> &bases) const
{
    if (bases.empty())
        return;

    std::vector<Mat4> instances;

    instances.reserve(bases.size());
    for (const DrawItem &item : _items)
    {
        Mat3 normalMatrix = glm::transpose(glm::inverse(Mat3(bases[0] * item.transform)));

        instances.clear();
        for (const Mat4 &base : bases)
            instances.push_back(base * item.transform);
        shader.setUniform("uNormalMatrix", normalMatrix);
        if (item.texture != nullptr)
        {
            item.texture->bind(0);
            shader.setUniform("uHasTexture", 1);
        }
        else
            shader.setUniform("uHasTexture", 0);
        item.mesh->drawInstanced(instances);
    }
}

Vec3 RenderModel::center() const
{
    return _center;
}

float RenderModel::radius() const
{
    return _radius;
}

Mat4 RenderModel::unitTransform() const
{
    return _unitTransform;
}

Mat4 RenderModel::footprintTransform() const
{
    return _footprintTransform;
}

} // namespace Zappy
