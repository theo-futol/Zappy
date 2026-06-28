#include "Graphics/modelslicer/ModelSlicer.hpp"

#include <glm/glm.hpp>

#include "types/Vec.hpp"

namespace Zappy
{

Model ModelSlicer::byTexture(const Model &full, const std::string &textureNeedle) const
{
    Model out;
    ModelNode root;

    root.transform = Mat4(1.0f);
    out.nodes.push_back(root);
    out.roots.push_back(0);
    for (const std::string &uri : full.textures)
        if (uri.find(textureNeedle) != std::string::npos)
        {
            out.textures.push_back(uri);
            break;
        }
    for (std::size_t r : full.roots)
        collect(full, r, Mat4(1.0f), textureNeedle, out);
    return out;
}

void ModelSlicer::collect(const Model &full, std::size_t nodeIndex, const Mat4 &parent, const std::string &needle, Model &out) const
{
    const ModelNode &node = full.nodes[nodeIndex];
    Mat4 world = parent * node.transform;
    Mat3 normalMatrix = glm::transpose(glm::inverse(Mat3(world)));

    for (std::size_t primitive : node.primitives)
    {
        const ModelPrimitive &source = full.primitives[primitive];

        if (source.texture < 0 || full.textures[source.texture].find(needle) == std::string::npos)
            continue;
        out.nodes[0].primitives.push_back(out.primitives.size());
        out.primitives.push_back(bake(source, world, normalMatrix));
    }
    for (std::size_t child : node.children)
        collect(full, child, world, needle, out);
}

ModelPrimitive ModelSlicer::bake(const ModelPrimitive &source, const Mat4 &world, const Mat3 &normalMatrix) const
{
    ModelPrimitive result;
    std::size_t positions = source.positions.size() / 3;
    std::size_t normals = source.normals.size() / 3;

    result.indices = source.indices;
    result.texcoords = source.texcoords;
    result.texture = 0;
    result.positions.reserve(source.positions.size());
    result.normals.reserve(source.normals.size());
    for (std::size_t i = 0; i < positions; ++i)
    {
        Vec3 point(source.positions[i * 3], source.positions[i * 3 + 1], source.positions[i * 3 + 2]);
        Vec3 worldPoint(world * Vec4(point, 1.0f));

        result.positions.push_back(worldPoint.x);
        result.positions.push_back(worldPoint.y);
        result.positions.push_back(worldPoint.z);
    }
    for (std::size_t i = 0; i < normals; ++i)
    {
        Vec3 normal(source.normals[i * 3], source.normals[i * 3 + 1], source.normals[i * 3 + 2]);
        Vec3 worldNormal = glm::normalize(normalMatrix * normal);

        result.normals.push_back(worldNormal.x);
        result.normals.push_back(worldNormal.y);
        result.normals.push_back(worldNormal.z);
    }
    return result;
}

} // namespace Zappy
