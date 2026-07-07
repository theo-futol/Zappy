#pragma once

#include "types/Mat.hpp"
#include "types/Ray.hpp"
#include "types/Vec.hpp"

namespace Zappy
{

/**
 * @class VRTabletop
 * @brief Places the flat 2D map as a small walkable-around diorama board in VR.
 *
 * PlanarProjection/MapRenderer/EntityRenderer are untouched: they still place tiles/entities at
 * grid-space positions like (x, 0, y). VRTabletop only supplies the extra transform (translate to
 * a spot in the room, scale down to a tabletop footprint) that RenderSystem's VR path folds into
 * the view matrix it hands to RenderContext, so the existing 2D renderers draw onto the table
 * without any changes of their own.
 */
class VRTabletop
{
  public:
    /**
     * @struct Hit
     * @brief Result of pickTile(): the grid tile a ray hits on the table surface, if any.
     */
    struct Hit
    {
        bool found; ///< False if the ray misses the table plane or falls outside the map bounds.
        int tileX;  ///< Hit tile column (valid only if found).
        int tileY;  ///< Hit tile row (valid only if found).
    };

    VRTabletop();

    /**
     * @brief Centers the table in front of the rig and picks a scale so the whole map fits it.
     * @param mapWidth Map width in tiles.
     * @param mapHeight Map height in tiles.
     */
    void autoFrame(int mapWidth, int mapHeight);

    /**
     * @brief Resizes the table (grip+thumbstick resize), clamped to a sane range.
     * @param factor Multiplicative scale step (>1 grows the table, <1 shrinks it).
     */
    void zoomBy(float factor);

    /**
     * @brief The transform placing grid-space positions onto the table in room space.
     * @return translate(center) * scale(worldUnitsPerTile) * translate(-gridCenter).
     */
    Mat4 tableModel() const;

    /**
     * @brief Intersects a world-space ray with the table's (horizontal) plane and maps it to a tile.
     * @param ray World-space ray, typically a hand's aim pose.
     * @return The tile under the ray, or a Hit with found=false if it misses the plane or the map.
     */
    Hit pickTile(const Ray &ray) const;

  private:
    static constexpr float TargetFootprintMeters = 0.8f; ///< Physical size of the map's largest dimension once framed.
    static constexpr float MinScale = 0.01f;              ///< Smallest allowed world-units-per-tile.
    static constexpr float MaxScale = 1.0f;               ///< Largest allowed world-units-per-tile.

    Vec3 _center;    ///< World-space (room-space) center of the table surface.
    float _scale;     ///< World units per grid tile.
    int _mapWidth;    ///< Map width in tiles, set by autoFrame().
    int _mapHeight;   ///< Map height in tiles, set by autoFrame().
};

} // namespace Zappy
