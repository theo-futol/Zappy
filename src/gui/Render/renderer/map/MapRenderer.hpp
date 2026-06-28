#pragma once

#include <vector>

#include "Model/tile/Tile.hpp"
#include "Render/asset/AssetCache.hpp"
#include "interface/IRenderer.hpp"
#include "types/GridPosition.hpp"
#include "types/ResourceType.hpp"
#include "types/Vec.hpp"

namespace Zappy
{

/**
 * @class MapRenderer
 * @brief Draws the terrain grid and the resources lying on the tiles.
 */
class MapRenderer : public IRenderer
{
  public:
    /**
     * @brief Builds the map renderer.
     * @param assets Shared asset cache used to resolve visuals.
     */
    explicit MapRenderer(AssetCache &assets);

    /**
     * @brief Draws the terrain and ground resources.
     * @param context Per-frame rendering context.
     */
    void render(const RenderContext &context) override;

  private:
    /**
     * @brief Checkerboard base color of a tile.
     * @param gridX Tile column.
     * @param gridY Tile row.
     * @return The tile color.
     */
    Vec3 tileColor(int gridX, int gridY) const;

    /**
     * @brief Draws a small colored marker for each resource present on a tile.
     * @param shader Shader already bound with the frame matrices.
     * @param mesh Quad mesh reused for every marker.
     * @param tile Tile whose resources are drawn.
     * @param tilePosition World position of the tile center.
     */
    void drawResources(Shader &shader, Mesh &mesh, const Tile &tile, const Vec3 &tilePosition) const;

    /**
     * @brief Distinct display color of a resource type.
     * @param type Resource type.
     * @return The marker color.
     */
    Vec3 resourceColor(ResourceType type) const;

    /**
     * @brief Sub-tile offset placing the marker of resource index i in a small grid.
     * @param index Resource index (0..6).
     * @return The local offset (with a small +z so it sits above the tile).
     */
    Vec3 resourceOffset(int index) const;

    /**
     * @brief Spawns rings for new broadcasts and draws the active expanding ripples (2D).
     * @param shader Basic shader, already bound with the frame matrices.
     * @param mesh Quad mesh reused for each ripple.
     * @param context Per-frame context (broadcast list, mapping and time).
     */
    void drawBroadcasts(Shader &shader, Mesh &mesh, const RenderContext &context);

    /**
     * @struct BroadcastPing
     * @brief An in-flight broadcast ripple: where it started and when.
     */
    struct BroadcastPing
    {
        GridPosition origin; ///< Tile the ripple expands from.
        float start;         ///< Time the ripple began (seconds).
    };

    AssetCache &_assets;                 ///< Shared GPU resource cache.
    Mesh *_ring = nullptr;               ///< Flat ring mesh for broadcast ripples (owned by the AssetCache).
    std::vector<BroadcastPing> _pings;   ///< Active broadcast ripples being animated.
    long _seenBroadcastSeq = 0;          ///< Highest broadcast sequence already turned into a ripple.
};

} // namespace Zappy
