#pragma once

#include <memory>
#include <string>

#include "Graphics/cubemap/CubemapTexture.hpp"
#include "Graphics/mesh/Mesh.hpp"
#include "Render/asset/AssetCache.hpp"
#include "interface/IRenderer.hpp"

namespace Zappy
{

/**
 * @class SkyRenderer
 * @brief Draws the surrounding sky from a cube-map, behind everything else.
 *
 * Renders a unit cube centered on the camera, sampling the cube map in the
 * direction of each vertex. The view matrix is stripped of its translation so
 * the sky never gets closer, and depth writes are disabled while drawing so the
 * scene always paints on top. Meant to run first in the render order.
 */
class SkyRenderer : public IRenderer
{
  public:
    /**
     * @brief Builds the sky renderer: loads the cube map, the cube mesh and the skybox shader.
     * @param assets Shared asset cache owning the mesh and shader.
     * @param cubemapPath Path to the 4x3 cross sky image.
     * @throws CubemapTexture::CubemapException On a cube-map loading failure.
     * @throws AssetCache::AssetCacheException On a mesh or shader failure.
     */
    SkyRenderer(AssetCache &assets, const std::string &cubemapPath);

    /**
     * @brief Draws the sky for the frame.
     * @param context Per-frame rendering context (the camera matrices are used).
     */
    void render(const RenderContext &context) override;

  private:
    AssetCache &_assets;                      ///< Shared GPU resource cache.
    std::unique_ptr<CubemapTexture> _cubemap; ///< Owned sky cube map.
    Mesh *_cube;                              ///< Cube mesh (owned by the AssetCache).
};

} // namespace Zappy
