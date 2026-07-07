#pragma once

#include <string>

#include "Graphics/text/TextRenderer.hpp"
#include "Render/asset/AssetCache.hpp"
#include "interface/IRenderer.hpp"
#include "types/Vec.hpp"

namespace Zappy
{

class GameState;

/**
 * @class HudRenderer
 * @brief Draws the styled HUD overlay: a right-side panel (background, accents,
 * team color swatches, sprite placeholders) with time unit, teams and winner labels.
 */
class HudRenderer : public IRenderer
{
  public:
    /// @brief Fraction of the window width taken by the HUD panel (the world uses the rest).
    static constexpr float PanelRatio = 0.40f;

    /**
     * @brief Width of the HUD panel in pixels for a given window width.
     * @param windowWidth Window width in pixels.
     * @return Panel (right strip) width in pixels.
     */
    static int panelWidth(int windowWidth);

    /**
     * @brief Builds the HUD renderer.
     * @param assets Asset cache (basic shader + quad mesh for the panel rectangles).
     * @param text Text renderer for the overlay labels.
     */
    HudRenderer(AssetCache &assets, TextRenderer &text);

    /**
     * @brief Updates the screen size so the panel stays on the right edge, full height.
     * @param width Window width in pixels.
     * @param height Window height in pixels.
     */
    void setScreenSize(int width, int height);

    /**
     * @brief Draws the styled HUD overlay for the current frame.
     * @param context Per-frame rendering context.
     */
    void render(const RenderContext &context) override;

    /**
     * @brief Routes a click to a HUD button and returns the command to send, if any.
     * @param px Click X in pixels (window top-left origin).
     * @param py Click Y in pixels (window top-left origin).
     * @param state State read to compute the new value (current time unit).
     * @return The command line to send, or an empty string if no button was hit.
     */
    std::string handleClick(double px, double py, const GameState &state) const;

    /**
     * @brief Whether a click hit the "MENU" button (return to the main menu).
     * @param px Click X in pixels (window top-left origin).
     * @param py Click Y in pixels (window top-left origin).
     * @return True if the menu button was clicked.
     */
    bool menuButtonHit(double px, double py) const;

  private:
    /**
     * @struct Button
     * @brief A clickable HUD rectangle in screen space (origin bottom-left).
     */
    struct Button
    {
        float x;      ///< Left edge in pixels.
        float y;      ///< Bottom edge in pixels.
        float width;  ///< Width in pixels.
        float height; ///< Height in pixels.
    };

    static constexpr float Margin = 22.0f;        ///< Inner padding of the panel.
    static constexpr float LineHeight = 30.0f;    ///< Vertical step between text lines.
    static constexpr float SectionGap = 16.0f;    ///< Extra vertical gap inserted between HUD sections.
    static constexpr float TitleHeight = 64.0f;   ///< Height of the top title bar.
    static constexpr float SwatchSize = 16.0f;    ///< Side of a team color swatch.
    static constexpr float SeparatorWidth = 3.0f; ///< Width of the left accent line.
    static constexpr float LogoSize = 36.0f;      ///< Placeholder logo/icon box side.
    static constexpr float ButtonSize = 26.0f;    ///< Side of a time-unit button.
    static constexpr float MenuButtonWidth = 84.0f; ///< Width of the MENU button.
    static constexpr float ButtonGap = 8.0f;      ///< Gap between the two time-unit buttons.
    static constexpr int MinTimeUnit = 1;         ///< Lowest time unit a button can request.
    static constexpr int MaxTimeUnit = 1000;      ///< Highest time unit a button can request (server caps at 1000).

    static constexpr Vec3 ButtonColor = Vec3(0.20f, 0.22f, 0.28f); ///< Time-unit button fill.

    static constexpr Vec3 PanelColor = Vec3(0.09f, 0.10f, 0.13f);       ///< Panel background.
    static constexpr Vec3 AccentColor = Vec3(0.95f, 0.55f, 0.15f);      ///< Title bar / accent line.
    static constexpr Vec3 PlaceholderColor = Vec3(0.22f, 0.24f, 0.30f); ///< Future sprite/icon slots.

    /**
     * @brief Draws a filled screen-space rectangle with the bound basic shader.
     * @param shader Shader already bound with the screen-space matrices.
     * @param mesh Unit quad mesh.
     * @param x Left edge in pixels.
     * @param y Bottom edge in pixels.
     * @param width Rectangle width in pixels.
     * @param height Rectangle height in pixels.
     * @param color Fill color.
     */
    void drawRect(Shader &shader, Mesh &mesh, float x, float y, float width, float height, const Vec3 &color) const;

    /** @brief Y (bottom-left origin) of the time-unit line, derived from the window height. @return The line Y. */
    float headerLineY() const;

    /** @brief Rectangle of the "decrease time unit" button. @return The button. */
    Button minusButton() const;

    /** @brief Rectangle of the "increase time unit" button. @return The button. */
    Button plusButton() const;

    /** @brief Rectangle of the "MENU" button in the panel title bar. @return The button. */
    Button menuButton() const;

    /**
     * @brief Hit-test of a button against a point (bottom-left origin).
     * @param button Button rectangle.
     * @param px Point X in pixels.
     * @param py Point Y in pixels (bottom-left origin).
     * @return True if the point is inside the button.
     */
    static bool contains(const Button &button, float px, float py);

    AssetCache &_assets; ///< Shader + quad for the panel rectangles.
    TextRenderer &_text; ///< Text renderer for the overlay labels.
    int _width;          ///< Window width in pixels.
    int _height;         ///< Window height in pixels.
};

} // namespace Zappy
