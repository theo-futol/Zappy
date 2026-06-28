#pragma once

#include <string>

#include "Graphics/mesh/Mesh.hpp"
#include "Graphics/shader/Shader.hpp"
#include "Graphics/text/TextRenderer.hpp"
#include "Render/asset/AssetCache.hpp"
#include "interface/IRenderer.hpp"
#include "types/Color.hpp"
#include "types/Vec.hpp"

namespace Zappy
{

class IEntity;
class GameState;

/**
 * @class SceneHudRenderer
 * @brief Screen-space HUD for the 3D view: a bottom info bar and a floating selection card.
 *
 * The bottom bar shows the global state (time unit, teams, winner, last log line).
 * When an entity is selected, a card floats next to it (its world position projected
 * to the screen) showing its info lines. Drawn in an orthographic screen space, on
 * top of the 3D scene with depth testing off.
 */
class SceneHudRenderer : public IRenderer
{
  public:
    /**
     * @brief Builds the 3D HUD renderer.
     * @param assets Asset cache (basic shader + quad mesh for the panels).
     * @param text Text renderer for the labels.
     */
    SceneHudRenderer(AssetCache &assets, TextRenderer &text);

    /**
     * @brief Updates the screen size the HUD lays itself out against.
     * @param width Window width in pixels.
     * @param height Window height in pixels.
     */
    void setScreenSize(int width, int height);

    /**
     * @brief Draws the HUD for the current frame.
     * @param context Per-frame rendering context (state and camera matrices).
     */
    void render(const RenderContext &context) override;

    /**
     * @brief Routes a click to the time-unit buttons and returns the command to send, if any.
     * @param px Click X in pixels (window top-left origin).
     * @param py Click Y in pixels (window top-left origin).
     * @param state State read to compute the new time unit.
     * @return The command line to send, or an empty string if no button was hit.
     */
    std::string handleClick(double px, double py, const GameState &state) const;

    /**
     * @brief Tells the HUD whether the camera is currently following an entity (button label).
     * @param following True while following.
     */
    void setFollowing(bool following);

    /**
     * @brief Tells the HUD whether the follow camera is in POV mode (button label).
     * @param pov True for first-person POV, false for third-person.
     */
    void setPov(bool pov);

    /**
     * @brief Tells the HUD which 3D camera mode is active (button label).
     * @param free True for free-fly, false for orbit.
     */
    void setCameraFree(bool free);

    /**
     * @brief Whether a click hit the camera-mode (orbit/free-fly) button.
     * @param px Click X in pixels (window top-left origin).
     * @param py Click Y in pixels (window top-left origin).
     * @return True if the camera-mode button was clicked.
     */
    bool cameraButtonHit(double px, double py) const;

    /**
     * @brief Whether a click hit the top-left "MENU" button (return to the main menu).
     * @param px Click X in pixels (window top-left origin).
     * @param py Click Y in pixels (window top-left origin).
     * @return True if the menu button was clicked.
     */
    bool menuButtonHit(double px, double py) const;

    /**
     * @brief Whether a click hit the follow/detach button of the state panel.
     * @param px Click X in pixels (window top-left origin).
     * @param py Click Y in pixels (window top-left origin).
     * @param state State read to know the selected entity.
     * @return True if the follow/detach button was clicked.
     */
    bool followButtonHit(double px, double py, const GameState &state) const;

    /**
     * @brief Whether a click hit the view-mode (3rd-person/POV) button of the state panel.
     * @param px Click X in pixels (window top-left origin).
     * @param py Click Y in pixels (window top-left origin).
     * @param state State read to know the selected entity.
     * @return True if the mode button was clicked.
     */
    bool modeButtonHit(double px, double py, const GameState &state) const;

  private:
    /**
     * @struct Button
     * @brief A clickable rectangle in screen space (bottom-left origin).
     */
    struct Button
    {
        float x;      ///< Left edge in pixels.
        float y;      ///< Bottom edge in pixels.
        float width;  ///< Width in pixels.
        float height; ///< Height in pixels.
    };
    /**
     * @brief Draws a filled screen-space rectangle.
     * @param shader Basic shader, already bound with the ortho projection.
     * @param mesh Unit quad mesh.
     * @param x Left edge in pixels.
     * @param y Bottom edge in pixels.
     * @param width Rectangle width in pixels.
     * @param height Rectangle height in pixels.
     * @param color Fill color.
     */
    void drawRect(Shader &shader, Mesh &mesh, float x, float y, float width, float height, const Vec3 &color) const;

    /**
     * @brief Draws the bottom info bar (time unit, teams, winner, last log line).
     * @param context Per-frame rendering context.
     */
    void drawBar(const RenderContext &context) const;

    /**
     * @brief Draws the fixed state panel (below the time box) for the selected entity, if any.
     * @param context Per-frame rendering context.
     */
    void drawStatePanel(const RenderContext &context) const;

    /**
     * @brief Draws the bottom-left tile panel (resources of the selected tile), if any.
     * @param context Per-frame rendering context.
     */
    void drawTilePanel(const RenderContext &context) const;

    /**
     * @brief Draws the top-right time-unit box (value and -/+ buttons).
     * @param context Per-frame rendering context.
     */
    void drawTimeBox(const RenderContext &context) const;

    /** @brief Draws the top-left "MENU" button. */
    void drawMenuButton() const;

    /**
     * @brief Draws the centered end-of-game banner (big "VICTOIRE" + winning team), if any.
     * @param context Per-frame rendering context.
     */
    void drawVictoryBanner(const RenderContext &context) const;

    /** @brief Rectangle of the "decrease time unit" button. @return The button. */
    Button minusButton() const;

    /** @brief Rectangle of the "increase time unit" button. @return The button. */
    Button plusButton() const;

    /** @brief Rectangle of the camera-mode (orbit/free-fly) button, just below the time box. @return The button. */
    Button cameraButton() const;

    /** @brief Rectangle of the top-left "MENU" button. @return The button. */
    Button menuButton() const;

    /**
     * @brief Total height of the state panel for a selected entity (lines + buttons).
     * @param selected Selected entity.
     * @return The panel height in pixels.
     */
    float panelHeightFor(const IEntity &selected) const;

    /**
     * @brief Bottom edge (in pixels) of the state panel for a selected entity.
     * @param selected Selected entity.
     * @return The panel bottom Y in pixels.
     */
    float panelBottomFor(const IEntity &selected) const;

    /**
     * @brief Rectangle of the follow/detach button (bottom row of the state panel).
     * @param selected Selected entity (sets the panel geometry).
     * @return The button rectangle.
     */
    Button followButton(const IEntity &selected) const;

    /**
     * @brief Rectangle of the view-mode button (second row, shown only while following).
     * @param selected Selected entity (sets the panel geometry).
     * @return The button rectangle.
     */
    Button modeButton(const IEntity &selected) const;

    /**
     * @brief Whether a screen point is inside a button.
     * @param button Button rectangle.
     * @param px Point X in pixels (bottom-left origin).
     * @param py Point Y in pixels (bottom-left origin).
     * @return True if the point is inside.
     */
    static bool contains(const Button &button, float px, float py);

    /**
     * @brief Converts an opaque RGB color to a Color for text drawing.
     * @param color RGB color.
     * @return The same color with full alpha.
     */
    static Color toColor(const Vec3 &color);

    static constexpr float BarHeight = 56.0f;    ///< Height of the bottom bar in pixels.
    static constexpr float Margin = 16.0f;       ///< Inner padding in pixels.
    static constexpr float LineHeight = 24.0f;   ///< Vertical step between text lines.
    static constexpr float SwatchSize = 14.0f;   ///< Side of a team color swatch.
    static constexpr float TeamStep = 130.0f;    ///< Horizontal step between teams in the bar.
    static constexpr float PanelWidth = 230.0f;  ///< Width of the state panel.
    static constexpr float CardPadding = 10.0f;  ///< Inner padding of the state panel.
    static constexpr float BoxWidth = 172.0f;    ///< Width of the top-right time box.
    static constexpr float BoxHeight = 72.0f;    ///< Height of the top-right time box.
    static constexpr float ButtonSize = 26.0f;   ///< Side of a time-unit button.
    static constexpr float MenuButtonWidth = 90.0f; ///< Width of the top-left MENU button.
    static constexpr int MinTimeUnit = 1;        ///< Lowest time unit a button can request.
    static constexpr int MaxTimeUnit = 1000;     ///< Highest time unit a button can request (server caps at 1000).

    static const Vec3 BarColor;    ///< Bottom bar background color.
    static const Vec3 AccentColor; ///< Accent line/title color.
    static const Vec3 CardColor;   ///< Selection card background color.
    static const Vec3 LabelColor;  ///< Default label text color.
    static const Vec3 ButtonColor; ///< Time-unit button fill.

    AssetCache &_assets;  ///< Shared GPU resource cache.
    TextRenderer &_text;  ///< Text renderer for labels.
    int _width;           ///< Window width in pixels.
    int _height;          ///< Window height in pixels.
    bool _following;      ///< Whether the camera follows an entity (sets the button label).
    bool _pov;            ///< Whether the follow camera is in POV mode (sets the mode button label).
    bool _cameraFree;     ///< Whether the 3D camera is in free-fly mode (sets the camera button label).
};

} // namespace Zappy
