#pragma once

#include <string>

#include "Render/asset/AssetCache.hpp"
#include "interface/IRenderer.hpp"
#include "types/Orientation.hpp"

namespace Zappy
{

/**
 * @class EntityRenderer
 * @brief Draws every on-map entity uniformly via its declared appearance.
 *
 * Iterates the homogeneous entity collection, resolves each appearance through
 * the asset cache and places it via the projection. Adding an entity type never
 * changes this renderer.
 */
class EntityRenderer : public IRenderer
{
  public:
    /**
     * @brief Builds the entity renderer.
     * @param assets Shared asset cache used to resolve visuals.
     */
    explicit EntityRenderer(AssetCache &assets);

    /**
     * @brief Draws all entities for the current frame.
     * @param context Per-frame rendering context.
     */
    void render(const RenderContext &context) override;

  private:
    /**
     * @brief Resolves which mesh draws an entity from its visual id.
     * @param visualId Symbolic visual identifier (e.g. "trantorian", "egg").
     * @return The mesh id to draw it with.
     */
    std::string meshForVisual(const std::string &visualId) const;

    /**
     * @brief Z-axis rotation angle (radians) for a facing orientation.
     * @param orientation Entity orientation.
     * @return The rotation to apply so a +Y-pointing mesh faces that way.
     */
    float angleForOrientation(Orientation orientation) const;

    AssetCache &_assets; ///< Shared GPU resource cache.
};

} // namespace Zappy
