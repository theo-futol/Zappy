#include "Render/rendermodel/RenderModel.hpp"

#include <algorithm>
#include <limits>
#include <string>

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Zappy
{

namespace
{

/**
 * @brief Offset (in floats) of keyframe `key`'s value inside a channel's values array.
 *
 * Cubic-spline output stores three vectors per keyframe (in-tangent, value, out-tangent);
 * we keep only the middle value, so cubic strides by 3 and skips the in-tangent.
 */
std::size_t valueOffset(const ModelAnimationChannel &channel, std::size_t key)
{
    std::size_t stride = (channel.interp == AnimationInterp::Cubic) ? 3 : 1;
    std::size_t bias = (channel.interp == AnimationInterp::Cubic) ? 1 : 0;

    return (key * stride + bias) * channel.components;
}

/** @brief Reads keyframe `key` as a vec3 (translation/scale). */
Vec3 readVec3(const ModelAnimationChannel &channel, std::size_t key)
{
    std::size_t o = valueOffset(channel, key);

    return Vec3(channel.values[o], channel.values[o + 1], channel.values[o + 2]);
}

/** @brief Reads keyframe `key` as a quaternion (rotation), converting glTF xyzw to glm wxyz. */
glm::quat readQuat(const ModelAnimationChannel &channel, std::size_t key)
{
    std::size_t o = valueOffset(channel, key);

    return glm::quat(channel.values[o + 3], channel.values[o], channel.values[o + 1], channel.values[o + 2]);
}

/**
 * @brief Samples a channel at `time` and writes the result into the node's TRS slots.
 * @param channel Source channel.
 * @param time Time within the clip (seconds).
 * @param translations Per-node translation, overwritten if the channel animates translation.
 * @param rotations Per-node rotation, overwritten if the channel animates rotation.
 * @param scales Per-node scale, overwritten if the channel animates scale.
 */
void sampleChannel(const ModelAnimationChannel &channel, float time, std::vector<Vec3> &translations, std::vector<glm::quat> &rotations, std::vector<Vec3> &scales)
{
    std::size_t count = channel.times.size();

    if (count == 0 || channel.node >= translations.size())
        return;

    std::size_t key = 0;

    while (key + 1 < count && channel.times[key + 1] < time)
        ++key;

    std::size_t next = (key + 1 < count) ? key + 1 : key;
    float span = channel.times[next] - channel.times[key];
    float alpha = (span > 0.0f) ? glm::clamp((time - channel.times[key]) / span, 0.0f, 1.0f) : 0.0f;
    bool step = (channel.interp == AnimationInterp::Step);

    if (channel.path == AnimationPath::Rotation)
    {
        glm::quat value = step ? readQuat(channel, key) : glm::slerp(readQuat(channel, key), readQuat(channel, next), alpha);

        rotations[channel.node] = glm::normalize(value);
        return;
    }

    Vec3 value = step ? readVec3(channel, key) : glm::mix(readVec3(channel, key), readVec3(channel, next), alpha);

    if (channel.path == AnimationPath::Translation)
        translations[channel.node] = value;
    else
        scales[channel.node] = value;
}

} // namespace

RenderModel::RenderModel(const Model &model, AssetCache &assets, const std::string &id, const std::string &directory)
    : _meshes(), _textures(), _localMin(), _localMax(), _items(), _unitTransform(1.0f), _footprintTransform(1.0f), _center(0.0f), _radius(1.0f), _skinned(false), _bindJoints()
{
    Vec3 boundsMin(std::numeric_limits<float>::max());
    Vec3 boundsMax(std::numeric_limits<float>::lowest());

    uploadPrimitives(model, assets, id);
    loadTextures(model, assets, id, directory);
    for (std::size_t root : model.roots)
        flattenNode(model, root, Mat4(1.0f), boundsMin, boundsMax);
    if (_items.empty())
        return;
    if (!model.skins.empty())
    {
        for (DrawItem &item : _items)
            item.transform = Mat4(1.0f);
        buildSkin(model);
        boundsMin = Vec3(std::numeric_limits<float>::max());
        boundsMax = Vec3(std::numeric_limits<float>::lowest());
        accumulateSkinnedBounds(model, boundsMin, boundsMax);
    }

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
    bool skinned = primitive.joints.size() >= count * 4 && primitive.weights.size() >= count * 4;

    data.vertices.reserve(count * 16);
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
        for (std::size_t k = 0; k < 4; ++k)
            data.vertices.push_back(skinned ? primitive.joints[i * 4 + k] : 0.0f);
        for (std::size_t k = 0; k < 4; ++k)
            data.vertices.push_back(skinned ? primitive.weights[i * 4 + k] : 0.0f);
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
            Mesh &mesh = assets.createMesh(id + "#" + std::to_string(i), data.vertices, data.indices, {3, 3, 2, 4, 4});

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

void RenderModel::accumulateGlobals(const Model &model, std::size_t nodeIndex, const Mat4 &parent, std::vector<Mat4> &out)
{
    Mat4 world = parent * model.nodes[nodeIndex].transform;

    out[nodeIndex] = world;
    for (std::size_t child : model.nodes[nodeIndex].children)
        accumulateGlobals(model, child, world, out);
}

void RenderModel::buildSkin(const Model &model)
{
    std::vector<Mat4> globals(model.nodes.size(), Mat4(1.0f));
    const ModelSkin &skin = model.skins[0];

    _skinned = true;
    for (std::size_t root : model.roots)
        accumulateGlobals(model, root, Mat4(1.0f), globals);
    _bindJoints.resize(skin.joints.size());
    for (std::size_t k = 0; k < skin.joints.size(); ++k)
        _bindJoints[k] = globals[skin.joints[k]] * skin.inverseBind[k];
    _nodes = model.nodes;
    _roots = model.roots;
    _skin = skin;
    _animations = model.animations;
}

void RenderModel::poseGlobals(std::size_t nodeIndex, const Mat4 &parent, const std::vector<Mat4> &locals, std::vector<Mat4> &out) const
{
    Mat4 world = parent * locals[nodeIndex];

    out[nodeIndex] = world;
    for (std::size_t child : _nodes[nodeIndex].children)
        poseGlobals(child, world, locals, out);
}

std::size_t RenderModel::animationCount() const
{
    return _animations.size();
}

int RenderModel::animationIndex(const std::string &name) const
{
    for (std::size_t i = 0; i < _animations.size(); ++i)
        if (_animations[i].name == name)
            return static_cast<int>(i);
    return -1;
}

float RenderModel::animationDuration(std::size_t animation) const
{
    if (animation >= _animations.size())
        return 0.0f;
    return _animations[animation].duration;
}

std::vector<Mat4> RenderModel::poseJoints(std::size_t animation, float time) const
{
    if (animation >= _animations.size())
        return _bindJoints;

    std::vector<Vec3> translations(_nodes.size());
    std::vector<glm::quat> rotations(_nodes.size());
    std::vector<Vec3> scales(_nodes.size());
    std::vector<bool> animated(_nodes.size(), false);

    for (std::size_t n = 0; n < _nodes.size(); ++n)
    {
        translations[n] = _nodes[n].translation;
        rotations[n] = _nodes[n].rotation;
        scales[n] = _nodes[n].scale;
    }
    for (const ModelAnimationChannel &channel : _animations[animation].channels)
    {
        sampleChannel(channel, time, translations, rotations, scales);
        if (channel.node < animated.size())
            animated[channel.node] = true;
    }

    std::vector<Mat4> locals(_nodes.size(), Mat4(1.0f));

    for (std::size_t n = 0; n < _nodes.size(); ++n)
        locals[n] = animated[n] ? glm::translate(Mat4(1.0f), translations[n]) * glm::mat4_cast(rotations[n]) * glm::scale(Mat4(1.0f), scales[n]) : _nodes[n].transform;

    std::vector<Mat4> globals(_nodes.size(), Mat4(1.0f));

    for (std::size_t root : _roots)
        poseGlobals(root, Mat4(1.0f), locals, globals);

    std::vector<Mat4> joints(_skin.joints.size());

    for (std::size_t k = 0; k < _skin.joints.size(); ++k)
        joints[k] = globals[_skin.joints[k]] * _skin.inverseBind[k];
    return joints;
}

void RenderModel::accumulateSkinnedBounds(const Model &model, Vec3 &boundsMin, Vec3 &boundsMax) const
{
    for (std::size_t p = 0; p < model.primitives.size(); ++p)
    {
        const ModelPrimitive &primitive = model.primitives[p];
        std::size_t count = primitive.positions.size() / 3;
        bool hasSkin = primitive.joints.size() >= count * 4 && primitive.weights.size() >= count * 4;

        if (!hasSkin)
        {
            boundsMin = glm::min(boundsMin, _localMin[p]);
            boundsMax = glm::max(boundsMax, _localMax[p]);
            continue;
        }
        for (std::size_t v = 0; v < count; ++v)
        {
            Vec4 position(primitive.positions[v * 3], primitive.positions[v * 3 + 1], primitive.positions[v * 3 + 2], 1.0f);
            Mat4 skin(0.0f);

            for (std::size_t k = 0; k < 4; ++k)
            {
                float weight = primitive.weights[v * 4 + k];
                std::size_t joint = static_cast<std::size_t>(primitive.joints[v * 4 + k]);

                if (weight > 0.0f && joint < _bindJoints.size())
                    skin += weight * _bindJoints[joint];
            }
            Vec3 posed(skin * position);

            boundsMin = glm::min(boundsMin, posed);
            boundsMax = glm::max(boundsMax, posed);
        }
    }
}

bool RenderModel::skinned() const
{
    return _skinned;
}

const std::vector<Mat4> &RenderModel::bindJoints() const
{
    return _bindJoints;
}

void RenderModel::drawSkinned(Shader &shader, const Mat4 &base, const std::vector<Mat4> &joints) const
{
    Mat3 normalMatrix = glm::transpose(glm::inverse(Mat3(base)));
    std::vector<Mat4> one{base};

    shader.setUniform("uSkinned", 1);
    for (std::size_t k = 0; k < joints.size(); ++k)
        shader.setUniform("uJoints[" + std::to_string(k) + "]", joints[k]);
    shader.setUniform("uNormalMatrix", normalMatrix);
    for (const DrawItem &item : _items)
    {
        if (item.texture != nullptr)
        {
            item.texture->bind(0);
            shader.setUniform("uHasTexture", 1);
        }
        else
            shader.setUniform("uHasTexture", 0);
        item.mesh->drawInstanced(one);
    }
    shader.setUniform("uSkinned", 0);
}

void RenderModel::drawInstanced(Shader &shader, const std::vector<Mat4> &bases) const
{
    if (bases.empty())
        return;

    std::vector<Mat4> instances;

    instances.reserve(bases.size());
    shader.setUniform("uSkinned", 0);
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
