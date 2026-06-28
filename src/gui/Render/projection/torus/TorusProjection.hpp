#pragma once

#include "interface/IProjection.hpp"
#include "types/WorldPoint.hpp"

namespace Zappy
{

/**
 * @class TorusProjection
 * @brief 3D-mode projection wrapping the (toroidal) Zappy grid onto a real 3D torus.
 *
 * The Zappy map already wraps around in both axes, so it is topologically a torus.
 * This projection maps each cell to a point on the inner wall of a torus tube:
 * grid X turns around the main ring, grid Y turns around the tube section. The
 * surface normal points inward (toward the tube centerline), so an entity placed
 * here stands on the inner wall with its head toward the tube's center — on the
 * top of the torus the head naturally points down.
 */
class TorusProjection : public IProjection
{
  public:
    TorusProjection();

    /**
     * @brief Sets the grid dimensions used to turn cells into torus angles.
     * @param width Grid width (cells around the main ring).
     * @param height Grid height (cells around the tube section).
     */
    void setGridSize(int width, int height);

    /**
     * @brief Sets the torus radii (must match the rendered torus asset).
     * @param major Major radius R (torus center to tube center).
     * @param minor Minor radius r (tube radius).
     */
    void setRadii(float major, float minor);

    /** @brief Major radius R. @return The major radius in world units. */
    float major() const;

    /** @brief Minor radius r. @return The minor radius in world units. */
    float minor() const;

    /**
     * @brief Maps a grid cell to a point on the torus inner wall.
     * @param gridX Column on the grid (angle around the main ring).
     * @param gridY Row on the grid (angle around the tube section).
     * @return The surface point, the inward normal (up) and the ring tangent (+X grid direction).
     */
    WorldPoint toWorld(int gridX, int gridY) const override;

  private:
    static constexpr float Tau = 6.28318530717958648f; ///< 2*pi.
    static constexpr float DefaultMajor = 12.5f;        ///< Default major radius (asset ratio ~4.15).
    static constexpr float DefaultMinor = 3.0f;         ///< Default minor radius (roomy enough for a trantorian).

    int _width;   ///< Grid width (cells around the main ring).
    int _height;  ///< Grid height (cells around the tube section).
    float _major; ///< Major radius R.
    float _minor; ///< Minor radius r.
};

} // namespace Zappy
