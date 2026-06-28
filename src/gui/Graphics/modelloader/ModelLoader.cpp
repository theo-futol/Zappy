#include "Graphics/modelloader/ModelLoader.hpp"

#include <memory>

#include <glm/gtc/type_ptr.hpp>

namespace Zappy
{

Model ModelLoader::load(const std::string &path) const
{
    cgltf_options options{};
    cgltf_data *raw = nullptr;

    if (cgltf_parse_file(&options, path.c_str(), &raw) != cgltf_result_success)
        throw ModelLoaderException("ModelLoader error: cannot parse " + path);

    std::unique_ptr<cgltf_data, decltype(&cgltf_free)> document(raw, &cgltf_free);

    if (cgltf_load_buffers(&options, document.get(), path.c_str()) != cgltf_result_success)
        throw ModelLoaderException("ModelLoader error: cannot load buffers of " + path);
    if (cgltf_validate(document.get()) != cgltf_result_success)
        throw ModelLoaderException("ModelLoader error: invalid glTF document " + path);

    Model model;
    std::vector<std::pair<std::size_t, std::size_t>> meshRanges = readMeshes(*document, model);

    readNodes(*document, meshRanges, model);
    return model;
}

std::vector<std::pair<std::size_t, std::size_t>> ModelLoader::readMeshes(const cgltf_data &data, Model &model) const
{
    std::vector<std::pair<std::size_t, std::size_t>> ranges;

    for (std::size_t mesh = 0; mesh < data.meshes_count; ++mesh)
    {
        std::size_t first = model.primitives.size();

        for (std::size_t prim = 0; prim < data.meshes[mesh].primitives_count; ++prim)
            model.primitives.push_back(readPrimitive(data.meshes[mesh].primitives[prim], model));
        ranges.push_back({first, model.primitives.size() - first});
    }
    return ranges;
}

ModelPrimitive ModelLoader::readPrimitive(const cgltf_primitive &primitive, Model &model) const
{
    ModelPrimitive result;

    result.positions = readAttribute(primitive, cgltf_attribute_type_position, Vec3Size);
    result.normals = readAttribute(primitive, cgltf_attribute_type_normal, Vec3Size);
    result.texcoords = readAttribute(primitive, cgltf_attribute_type_texcoord, Vec2Size);
    result.texture = resolveTexture(primitive, model);
    if (primitive.indices != nullptr)
    {
        result.indices.reserve(primitive.indices->count);
        for (std::size_t i = 0; i < primitive.indices->count; ++i)
            result.indices.push_back(static_cast<unsigned int>(cgltf_accessor_read_index(primitive.indices, i)));
    }
    else
    {
        std::size_t vertices = result.positions.size() / Vec3Size;

        for (std::size_t i = 0; i < vertices; ++i)
            result.indices.push_back(static_cast<unsigned int>(i));
    }
    return result;
}

std::vector<float> ModelLoader::readAttribute(const cgltf_primitive &primitive, cgltf_attribute_type type, std::size_t components) const
{
    std::vector<float> values;

    for (std::size_t a = 0; a < primitive.attributes_count; ++a)
    {
        const cgltf_attribute &attribute = primitive.attributes[a];

        if (attribute.type != type || attribute.index != 0 || attribute.data == nullptr)
            continue;
        values.resize(attribute.data->count * components);
        for (std::size_t i = 0; i < attribute.data->count; ++i)
            cgltf_accessor_read_float(attribute.data, i, &values[i * components], components);
        return values;
    }
    return values;
}

int ModelLoader::resolveTexture(const cgltf_primitive &primitive, Model &model) const
{
    if (primitive.material == nullptr)
        return -1;

    const cgltf_material &material = *primitive.material;
    const cgltf_texture *texture = nullptr;

    if (material.has_pbr_metallic_roughness)
        texture = material.pbr_metallic_roughness.base_color_texture.texture;
    if (texture == nullptr && material.has_pbr_specular_glossiness)
        texture = material.pbr_specular_glossiness.diffuse_texture.texture;
    if (texture == nullptr || texture->image == nullptr || texture->image->uri == nullptr)
        return -1;

    std::string uri = texture->image->uri;

    for (std::size_t i = 0; i < model.textures.size(); ++i)
        if (model.textures[i] == uri)
            return static_cast<int>(i);
    model.textures.push_back(uri);
    return static_cast<int>(model.textures.size() - 1);
}

void ModelLoader::readNodes(const cgltf_data &data, const std::vector<std::pair<std::size_t, std::size_t>> &meshRanges, Model &model) const
{
    model.nodes.resize(data.nodes_count);
    for (std::size_t n = 0; n < data.nodes_count; ++n)
    {
        const cgltf_node &source = data.nodes[n];
        ModelNode &node = model.nodes[n];
        float matrix[16] = {};

        if (source.name != nullptr)
            node.name = source.name;
        cgltf_node_transform_local(&source, matrix);
        node.transform = glm::make_mat4(matrix);
        if (source.mesh != nullptr)
        {
            std::size_t mesh = static_cast<std::size_t>(source.mesh - data.meshes);
            std::pair<std::size_t, std::size_t> range = meshRanges[mesh];

            for (std::size_t p = 0; p < range.second; ++p)
                node.primitives.push_back(range.first + p);
        }
        for (std::size_t c = 0; c < source.children_count; ++c)
            node.children.push_back(static_cast<std::size_t>(source.children[c] - data.nodes));
        if (source.parent == nullptr)
            model.roots.push_back(n);
    }
}

} // namespace Zappy
