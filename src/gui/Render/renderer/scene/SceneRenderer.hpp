#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Render/asset/AssetCache.hpp"
#include "Render/rendermodel/RenderModel.hpp"
#include "Render/theme/ThemeRegistry.hpp"
#include "interface/IRenderer.hpp"
#include "types/Mat.hpp"
#include "types/Model.hpp"
#include "types/Orientation.hpp"
#include "types/Vec.hpp"
#include "types/WorldPoint.hpp"

namespace Zappy
{

/**
 * @class SceneRenderer
 * @brief Draws the 3D scene (ground tiles and one model per entity) with the Phong shader.
 *
 * Holds two GPU models: a ground tile repeated on every map cell, and an entity
 * model placed at each entity's grid position. Sets the per-frame lighting and
 * camera uniforms, then lets each model draw its parts.
 */
class SceneRenderer : public IRenderer
{
  public:
    /**
     * @brief Builds the scene renderer: uploads the ground and every theme's models to the GPU.
     * @param assets Shared asset cache providing the Phong shader and owning the resources.
     * @param ground Decoded ground model, tiled over the map.
     * @param groundDirectory Directory of the ground model file (for its textures).
     */
    SceneRenderer(AssetCache &assets, const Model &ground, const std::string &groundDirectory);

    /**
     * @brief Draws the ground and the entities for the frame.
     * @param context Per-frame rendering context (camera matrices and mapping).
     */
    void render(const RenderContext &context) override;

    /**
     * @brief Sets the torus radii so the world shell is scaled to match the projection.
     * @param major Major radius R (world units).
     * @param minor Minor radius r (world units).
     */
    void setTorusRadii(float major, float minor);

    /**
     * @brief Selects the ground shape: torus planet shell (true) or flat tiled ground (false).
     * @param torusWorld True for the torus world, false for the flat ground.
     */
    void setTorusWorld(bool torusWorld);

  private:
    /**
     * @brief Loads each theme's golem and egg model, keyed by theme index.
     * @param assets Asset cache that will own the created meshes and textures.
     * @throws ModelLoader::ModelLoaderException On a model parsing failure.
     * @throws RenderModel::RenderModelException On a GPU upload failure.
     */
    void loadThemes(AssetCache &assets);

    /**
     * @brief Loads the resource models, keyed by ResourceType (food whole, stones sliced from the crystal pack).
     * @param assets Asset cache that will own the created meshes and textures.
     * @throws ModelLoader::ModelLoaderException On a model parsing failure.
     * @throws RenderModel::RenderModelException On a GPU upload failure.
     */
    void loadResources(AssetCache &assets);

    /**
     * @brief Directory part of a model path, used to resolve its relative textures.
     * @param path Full path to the model file.
     * @return The directory, or "." when the path has no directory part.
     */
    static std::string directoryOf(const std::string &path);

    /**
     * @brief In-tile offset where a resource type sits, so several types don't overlap.
     * @param index ResourceType index (0..ResourceSet::Count - 1).
     * @return A small ground-level (x, 0, z) offset within the tile.
     */
    static Vec3 resourceSpot(std::size_t index);

    /**
     * @brief Draws a flat colored marker on a tile, lying on the surface (hover/selection feedback).
     * @param shader Phong shader, already bound with the frame uniforms.
     * @param point Surface point of the tile (position + normal + tangent).
     * @param color Marker color.
     */
    void drawHighlight(Shader &shader, const WorldPoint &point, const Vec3 &color) const;

    /**
     * @brief Builds the orthonormal surface frame at a tile (tangent = +X, normal = +Y, bitangent = +Z).
     * @param point Surface point (position + normal + tangent) from the projection.
     * @return A rotation matrix mapping a model's local axes onto the surface.
     */
    static Mat4 tileFrame(const WorldPoint &point);

    /**
     * @brief Local yaw (degrees, around the surface normal) matching a server orientation.
     * @param orientation Entity orientation (North/East/South/West).
     * @return The facing yaw to apply within the surface frame.
     */
    static float orientationYaw(Orientation orientation);

    AssetCache &_assets;                  ///< Shared GPU resource cache.
    std::unique_ptr<RenderModel> _ground; ///< Ground model, tiled per map cell.
    ThemeRegistry _registry;              ///< Team-to-model assignment (golem + egg per theme).
    std::vector<std::unique_ptr<RenderModel>> _golems;    ///< Player model per theme (index = theme).
    std::vector<std::unique_ptr<RenderModel>> _eggs;      ///< Egg model per theme (index = theme).
    std::vector<std::unique_ptr<RenderModel>> _resources; ///< Resource model per ResourceType (index = type).
    Mesh *_highlight;                                     ///< Flat quad for tile markers (owned by the AssetCache).
    float _torusMajor;                                   ///< Torus major radius R (world units), set by the render system.
    float _torusMinor;                                   ///< Torus minor radius r (world units), set by the render system.
    bool _torusWorld;                                    ///< True to draw the torus shell, false for the flat tiled ground.

    static constexpr const char *CrystalModelPath = "assets/resources/crystal/scene.gltf"; ///< Crystal pack (sliced per stone).
    static constexpr const char *FoodModelPath = "assets/resources/food/scene.gltf";       ///< Food model (used whole).
    static constexpr float ResourceScale = 0.3f;                                           ///< Size of a resource model, in tiles.
    static constexpr float ResourceSpacing = 0.28f;                                        ///< In-tile spacing between resource spots.
    static constexpr float HighlightLift = 0.02f;                                          ///< Height of a tile marker above the ground.
    static constexpr float HighlightSize = 0.95f;                                          ///< Side of a tile marker, in tiles.

    static const Vec3 LightColor;   ///< Color/intensity of the light.
    static const Vec3 ModelColor;   ///< Base color used for untextured primitives.
    static const Vec3 HoverColor;   ///< Marker color of the hovered tile.
    static const Vec3 SelectColor;  ///< Marker color of the selected entity's tile.
};

} // namespace Zappy
