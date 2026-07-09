#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <glm/gtc/quaternion.hpp>

#include "types/Mat.hpp"
#include "types/Vec.hpp"

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
    std::vector<float> joints;         ///< Skin joint indices, 4 floats per vertex (empty if not skinned).
    std::vector<float> weights;        ///< Skin joint weights, 4 floats per vertex (empty if not skinned).
    std::vector<unsigned int> indices; ///< Triangle indices into the vertex arrays.
    int texture;                       ///< Index into Model::textures, or -1 when untextured.
    int emissive;                      ///< Index into Model::textures for the emissive map, or -1.
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
    Mat4 transform;                    ///< Local transform of this node (composed TRS, used by the static path).
    glm::vec3 translation{0.0f};       ///< Local translation (animation overrides this component).
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f}; ///< Local rotation (animation overrides this component).
    glm::vec3 scale{1.0f};             ///< Local scale (animation overrides this component).
    std::vector<std::size_t> primitives; ///< Indices into Model::primitives drawn at this node.
    std::vector<std::size_t> children;   ///< Indices into Model::nodes parented to this node.
    int skin = -1;                       ///< Index into Model::skins for the mesh drawn here, or -1 if not skinned.
};

/**
 * @enum AnimationPath
 * @brief Which node property a channel animates.
 */
enum class AnimationPath
{
    Translation, ///< Animates the node's local translation (vec3).
    Rotation,    ///< Animates the node's local rotation (quaternion).
    Scale        ///< Animates the node's local scale (vec3).
};

/**
 * @enum AnimationInterp
 * @brief How a channel's keyframes are interpolated.
 */
enum class AnimationInterp
{
    Linear, ///< Linear (lerp for vec3, slerp for quaternions).
    Step,   ///< Hold the previous keyframe (no interpolation).
    Cubic   ///< Cubic spline (sampled here as the keyframe value, tangents ignored).
};

/**
 * @struct ModelAnimationChannel
 * @brief One animated property of one node: its keyframe times and values.
 */
struct ModelAnimationChannel
{
    std::size_t node;            ///< Target node index (into Model::nodes).
    AnimationPath path;          ///< Which property is animated.
    AnimationInterp interp;      ///< Interpolation mode.
    std::vector<float> times;    ///< Keyframe times in seconds (K entries).
    std::vector<float> values;   ///< Keyframe values, components per key (K * components).
    std::size_t components;      ///< Values per key: 3 (translation/scale) or 4 (rotation).
};

/**
 * @struct ModelAnimation
 * @brief A named animation clip: a set of channels and a total duration.
 */
struct ModelAnimation
{
    std::string name;                          ///< Clip name (e.g. "transform_to_robot").
    float duration = 0.0f;                      ///< Length in seconds (max keyframe time).
    std::vector<ModelAnimationChannel> channels; ///< Per-node animated properties.
};

/**
 * @struct ModelSkin
 * @brief A skeleton: the joint nodes and their inverse bind matrices.
 *
 * `joints[k]` is the index (into Model::nodes) of the k-th joint; `inverseBind[k]`
 * maps a vertex from model space into that joint's local bind space. The skinning
 * matrix of joint k is `globalNodeTransform(joints[k]) * inverseBind[k]`.
 */
struct ModelSkin
{
    std::vector<std::size_t> joints; ///< Node index of each joint, in skin order.
    std::vector<Mat4> inverseBind;   ///< Inverse bind matrix of each joint (same order as joints).
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
    std::vector<ModelSkin> skins;           ///< All skeletons (empty for a static model).
    std::vector<ModelAnimation> animations; ///< All animation clips (empty if none).
};

} // namespace Zappy
