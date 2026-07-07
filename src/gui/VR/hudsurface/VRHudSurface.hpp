#pragma once

#include <glad/glad.h>

#include "Graphics/context/GraphicsContext.hpp"
#include "Render/asset/AssetCache.hpp"
#include "Render/rendercontext/RenderContext.hpp"
#include "interface/IRenderer.hpp"
#include "types/Mat.hpp"
#include "types/Ray.hpp"
#include "types/Vec.hpp"

namespace Zappy
{

/**
 * @class VRHudSurface
 * @brief Presents an existing screen-space HUD renderer on a world-space quad in VR.
 *
 * Renders the HUD (HudRenderer or SceneHudRenderer, both plain IRenderer) into an offscreen
 * framebuffer at its normal fixed pixel resolution: their pixel-rect drawing and hit-test
 * methods (menuButtonHit, handleClick, ...) never change. Only the presentation (a textured
 * world-space quad instead of the default framebuffer) and the input path (a controller-ray/quad
 * intersection converted back to pixel coordinates, instead of the OS mouse cursor) are new.
 */
class VRHudSurface
{
  public:
    /**
     * @brief Creates the offscreen framebuffer/texture and the presentation quad mesh/shader.
     * @param assets Shared GPU resource cache (for the quad mesh and the textured-quad shader).
     * @param context Graphics state helper, for toggling depth testing/viewport around the HUD pass.
     * @param pixelWidth HUD resolution width, in pixels (must match the HUD renderer's screen size).
     * @param pixelHeight HUD resolution height, in pixels.
     * @throws AssetCache::AssetCacheException If the presentation shader fails to compile.
     */
    VRHudSurface(AssetCache &assets, GraphicsContext &context, int pixelWidth, int pixelHeight);

    /** @brief Deletes the offscreen framebuffer/texture/renderbuffer. */
    ~VRHudSurface();

    VRHudSurface(const VRHudSurface &) = delete;
    VRHudSurface &operator=(const VRHudSurface &) = delete;
    VRHudSurface(VRHudSurface &&) = delete;
    VRHudSurface &operator=(VRHudSurface &&) = delete;

    /**
     * @brief Places the panel at a fixed world-space pose.
     * @param center World-space center of the panel.
     * @param forward World-space direction the panel faces (its outward normal); must not be near-vertical.
     * @param worldWidth Panel width in world units.
     * @param worldHeight Panel height in world units.
     */
    void setPose(const Vec3 &center, const Vec3 &forward, float worldWidth, float worldHeight);

    /**
     * @brief Renders the HUD into the offscreen framebuffer, unchanged from its desktop draw call.
     * @param hud The existing HUD renderer (HudRenderer or SceneHudRenderer) to draw as-is.
     * @param context Frame context passed straight through to the HUD's own render().
     */
    void renderHud(IRenderer &hud, const RenderContext &context);

    /**
     * @brief Draws the presentation quad, textured with the last renderHud() result.
     * @param view Current eye's view matrix.
     * @param projection Current eye's projection matrix.
     */
    void renderQuad(const Mat4 &view, const Mat4 &projection);

    /**
     * @brief Intersects a world-space ray with the panel's plane and converts the hit to HUD pixels.
     * @param ray World-space ray (typically a hand's aim pose).
     * @param outPixelX Receives the hit's X in HUD pixel coordinates.
     * @param outPixelY Receives the hit's Y in HUD pixel coordinates.
     * @return True if the ray hits within the panel's bounds, false otherwise.
     */
    bool hitTest(const Ray &ray, double &outPixelX, double &outPixelY) const;

  private:
    AssetCache &_assets;         ///< Shared GPU resource cache (quad mesh + presentation shader).
    GraphicsContext &_context;   ///< Graphics state helper (viewport/depth test around the HUD pass).
    int _pixelWidth;             ///< HUD framebuffer width in pixels.
    int _pixelHeight;            ///< HUD framebuffer height in pixels.
    GLuint _framebuffer;         ///< Offscreen FBO the HUD renders into.
    GLuint _colorTexture;        ///< Color attachment, sampled when presenting the quad.
    GLuint _depthRenderbuffer;   ///< Depth attachment (the HUD renderers toggle depth test off/on internally).
    Vec3 _center;                ///< World-space center of the panel.
    Vec3 _right;                 ///< World-space right axis of the panel (its U texture direction).
    Vec3 _up;                    ///< World-space up axis of the panel (its V texture direction).
    Vec3 _normal;                ///< World-space outward-facing normal of the panel.
    float _worldWidth;           ///< Panel width in world units.
    float _worldHeight;          ///< Panel height in world units.
};

} // namespace Zappy
