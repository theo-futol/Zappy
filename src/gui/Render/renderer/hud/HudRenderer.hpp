#pragma once

#include "Graphics/text/TextRenderer.hpp"
#include "interface/IRenderer.hpp"

namespace Zappy
{

/**
 * @class HudRenderer
 * @brief Draws the overlay: time unit, teams, winner and selected entity info.
 */
class HudRenderer : public IRenderer
{
  public:
    /**
     * @brief Builds the HUD renderer.
     * @param text Text renderer used for the overlay.
     */
    explicit HudRenderer(TextRenderer &text);

    /**
     * @brief Draws the overlay for the current frame.
     * @param context Per-frame rendering context.
     */
    void render(const RenderContext &context) override;

  private:
    TextRenderer &_text; ///< Text renderer for the overlay.
};

} // namespace Zappy
