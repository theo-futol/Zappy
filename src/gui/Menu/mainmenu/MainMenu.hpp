#pragma once

#include <exception>
#include <memory>
#include <set>
#include <string>

#include "Graphics/context/GraphicsContext.hpp"
#include "Graphics/text/TextRenderer.hpp"
#include "Graphics/window/Window.hpp"
#include "Render/asset/AssetCache.hpp"
#include "Render/rendermodel/RenderModel.hpp"
#include "types/Color.hpp"
#include "types/Mat.hpp"
#include "types/MenuConfig.hpp"
#include "types/RenderMode.hpp"
#include "types/Vec.hpp"

namespace Zappy
{

/**
 * @class MainMenu
 * @brief Full-screen configuration menu: host/port text fields and a 2D/3D toggle.
 *
 * Rendered into the shared window/context owned by Core. It runs its own blocking
 * loop until the user validates (returns the chosen settings) or quits. The menu
 * borrows the window and graphics context; it owns its own small asset cache and
 * text renderer, loaded once and reused across visits.
 */
class MainMenu
{
  public:
    /**
     * @class MainMenuException
     * @brief Error raised while loading the menu's assets.
     */
    class MainMenuException : public std::exception
    {
      public:
        explicit MainMenuException(const std::string &message) : _message(message)
        {
        }

        const char *what() const noexcept override
        {
            return _message.c_str();
        }

      private:
        std::string _message; ///< Error description.
    };

    /**
     * @brief Builds the menu on top of an existing window and context.
     * @param window Window owned by Core (borrowed).
     * @param context Graphics context owned by Core (borrowed).
     */
    MainMenu(Window &window, GraphicsContext &context);

    MainMenu(const MainMenu &) = delete;
    MainMenu &operator=(const MainMenu &) = delete;
    MainMenu(MainMenu &&) = delete;
    MainMenu &operator=(MainMenu &&) = delete;

    /**
     * @brief Loads the menu assets (basic shader, quad mesh, font).
     * @throws MainMenuException On an asset or font loading failure.
     */
    void init();

    /**
     * @brief Pre-fills the fields (e.g. from command-line arguments).
     * @param host Initial host string (empty to leave blank).
     * @param port Initial port (<= 0 to leave blank).
     * @param mode Initial display mode.
     */
    void setDefaults(const std::string &host, int port, RenderMode mode);

    /**
     * @brief Sets the error message shown under the buttons (e.g. a failed connection).
     * @param error Error text (empty to clear).
     */
    void setError(const std::string &error);

    /**
     * @brief Runs the menu loop until the user validates or quits.
     * @return The chosen settings, or a Quit result.
     */
    MenuConfig run();

  private:
    /**
     * @struct Rect
     * @brief A screen-space rectangle (bottom-left origin).
     */
    struct Rect
    {
        float x;      ///< Left edge in pixels.
        float y;      ///< Bottom edge in pixels.
        float width;  ///< Width in pixels.
        float height; ///< Height in pixels.
    };

    /**
     * @enum Field
     * @brief Which text field currently has keyboard focus.
     */
    enum class Field
    {
        None, ///< No field focused.
        Host, ///< The host field is focused.
        Port  ///< The port field is focused.
    };

    /** @brief Rectangle of the host text field. @return The field rectangle. */
    Rect hostField() const;

    /** @brief Rectangle of the port text field. @return The field rectangle. */
    Rect portField() const;

    /** @brief Rectangle of the "2D" mode button. @return The button rectangle. */
    Rect modeTwoButton() const;

    /** @brief Rectangle of the "3D flat" mode button. @return The button rectangle. */
    Rect modeThreeButton() const;

    /** @brief Rectangle of the "3D torus" mode button. @return The button rectangle. */
    Rect modeTorusButton() const;

    /** @brief Rectangle of the validate button. @return The button rectangle. */
    Rect validateButton() const;

    /** @brief Rectangle of the quit button. @return The button rectangle. */
    Rect quitButton() const;

    /** @brief X of the centered panel's left edge. @return The panel left X. */
    float panelLeft() const;

    /** @brief Y of the centered panel's bottom edge. @return The panel bottom Y. */
    float panelBottom() const;

    /**
     * @brief Routes a typed character to the focused field (with per-field filtering).
     * @param codepoint Unicode codepoint of the typed character.
     */
    void appendChar(int codepoint);

    /**
     * @brief Routes a mouse click to the fields, mode buttons and action buttons.
     * @param px Click X in pixels (window top-left origin).
     * @param py Click Y in pixels (window top-left origin).
     * @param config Receives the result on a validate/quit action.
     * @return True if the menu should close (config is filled), false otherwise.
     */
    bool handleClick(double px, double py, MenuConfig &config);

    /**
     * @brief Validates the fields and fills the config on success.
     * @param config Receives the settings if valid.
     * @return True if the settings are valid (menu can close), false otherwise.
     */
    bool tryValidate(MenuConfig &config);

    /**
     * @brief Loads the dropship model (best-effort: leaves _ship null on failure).
     *
     * The background is decorative, so a missing or broken model disables it
     * rather than aborting the menu.
     */
    void loadShip();

    /** @brief Loads the transition portal model (best-effort: leaves _portal null on failure). */
    void loadPortal();

    /**
     * @brief Sets the phong uniforms for the given camera and draws the ship once.
     * @param view Camera view matrix.
     * @param projection Camera projection matrix.
     * @param eye Camera position (used for the light and specular).
     * @param base Extra model->world transform applied before the ship's unit transform.
     */
    void renderShipModel(const Mat4 &view, const Mat4 &projection, const Vec3 &eye, const Mat4 &base);

    /**
     * @brief Begins the "fly into the portal" transition instead of returning immediately.
     * @param config The validated settings to return once the animation finishes.
     */
    void startTransition(const MenuConfig &config);

    /**
     * @brief Draws one frame of the validation transition (ship rushing into the portal + flash).
     * @param t Normalized progress in [0, 1].
     */
    void drawTransition(float t);

    /**
     * @brief Draws a full-screen white quad at the given opacity (end-of-transition flash).
     * @param alpha Opacity in [0, 1].
     */
    void drawFlash(float alpha);

    /**
     * @brief Enters free-fly debug mode: seeds the free camera from the current pose.
     *
     * Temporary tool to tune the cabin viewpoint: it copies the constant-based eye
     * and look direction into the free camera so movement starts from the same view.
     */
    void enterFreeFly();

    /**
     * @brief Integrates one frame of free-fly movement from the held keys and logs the pose.
     *
     * No-op unless free-fly is active. On any input it prints the eye as ready-to-paste
     * EyeX/EyeY/EyeZ (radius units) and LookX/Y/Z constants.
     */
    void updateFreeCamera();

    /** @brief Unit forward vector from the free camera's yaw/pitch. @return The look direction. */
    Vec3 freeForward() const;

    /** @brief Draws one frame of the menu. */
    void draw();

    /**
     * @brief Draws the 3D dropship behind the 2D UI, seen from inside the cabin.
     *
     * Runs with depth-test on and a perspective camera placed at the model's
     * bounding-sphere center; the UI is then painted on top in ortho. No-op if
     * the model failed to load.
     */
    void drawShip();

    /**
     * @brief Draws a filled screen-space rectangle (optionally translucent).
     * @param rect Rectangle to fill.
     * @param color Fill color.
     * @param alpha Opacity in [0, 1] (1 = opaque).
     */
    void drawRect(const Rect &rect, const Vec3 &color, float alpha = 1.0f);

    /**
     * @brief Draws a hollow rectangular border of the given thickness.
     * @param rect Outer rectangle.
     * @param color Border color.
     * @param thickness Border thickness in pixels.
     * @param alpha Opacity in [0, 1].
     */
    void drawOutline(const Rect &rect, const Vec3 &color, float thickness, float alpha = 1.0f);

    /**
     * @brief Draws L-shaped tech brackets at the four corners of a rectangle.
     * @param rect Rectangle whose corners to frame.
     * @param length Length of each bracket arm in pixels.
     * @param thickness Arm thickness in pixels.
     * @param color Bracket color.
     */
    void drawCornerBrackets(const Rect &rect, float length, float thickness, const Vec3 &color);

    /**
     * @brief Draws text horizontally centered on a given x.
     * @param text Text to draw.
     * @param centerX Center x in pixels.
     * @param y Baseline y in pixels.
     * @param color Text color.
     * @param scale Glyph scale factor.
     */
    void drawCenteredText(const std::string &text, float centerX, float y, const Color &color, float scale = 1.0f);

    /**
     * @brief Draws a mode-toggle button (outlined, amber-filled when selected).
     * @param rect Button rectangle.
     * @param label Button caption.
     * @param selected Whether this mode is the current one.
     */
    void drawModeButton(const Rect &rect, const std::string &label, bool selected);

    /**
     * @brief Draws an action button (primary = amber accent, secondary = steel).
     * @param rect Button rectangle.
     * @param label Button caption.
     * @param primary Whether this is the primary action.
     */
    void drawActionButton(const Rect &rect, const std::string &label, bool primary);

    /**
     * @brief Draws a text field (background, focus border, label and value).
     * @param rect Field rectangle.
     * @param label Label drawn above the field.
     * @param value Current text value.
     * @param focused Whether the field has focus.
     */
    void drawTextField(const Rect &rect, const std::string &label, const std::string &value, bool focused);

    /**
     * @brief Whether a screen point is inside a rectangle.
     * @param rect Rectangle.
     * @param px Point X in pixels (bottom-left origin).
     * @param py Point Y in pixels (bottom-left origin).
     * @return True if inside.
     */
    static bool contains(const Rect &rect, float px, float py);

    /**
     * @brief Converts an opaque RGB color to a Color for text drawing.
     * @param color RGB color.
     * @return The same color with full alpha.
     */
    static Color toColor(const Vec3 &color);

    static constexpr float PanelWidth = 460.0f;   ///< Width of the centered menu panel.
    static constexpr float PanelHeight = 480.0f;  ///< Height of the centered menu panel (extra room below QUITTER).
    static constexpr float FieldWidth = 380.0f;   ///< Width of a text field / wide button.
    static constexpr float FieldHeight = 44.0f;   ///< Height of a text field.
    static constexpr float ButtonHeight = 48.0f;  ///< Height of an action button.
    static constexpr float RowGap = 78.0f;        ///< Vertical step between stacked rows.
    static constexpr float Pad = 40.0f;           ///< Inner padding of the panel.
    static constexpr int Backspace = 259;         ///< GLFW key code for Backspace.
    static constexpr int Enter = 257;             ///< GLFW key code for Enter.
    static constexpr int Tab = 258;               ///< GLFW key code for Tab.
    static constexpr int MaxPort = 65535;         ///< Highest valid TCP port.

    // --- 3D dropship background (tune these live in step 1b) ---
    static constexpr const char *ShipModelPath = "assets/menu/scene.gltf"; ///< glTF dropship scene.
    static constexpr const char *ShipModelDir = "assets/menu";             ///< Dir for its textures.
    static constexpr float ShipFov = 70.0f;   ///< Vertical field of view, degrees.
    static constexpr float ShipNear = 0.01f;  ///< Near clip plane (small: camera is inside).
    static constexpr float ShipFar = 50.0f;   ///< Far clip plane.
    // Eye placed in the cabin (tuned via the free-fly debug camera).
    static constexpr float EyeX = -0.026f;    ///< Eye offset from center, in radius units (right).
    static constexpr float EyeY = 0.077f;     ///< Eye offset from center, in radius units (up).
    static constexpr float EyeZ = 0.144f;     ///< Eye offset from center, in radius units (forward).
    static constexpr float LookX = 0.768f;    ///< Base look direction X (from the eye).
    static constexpr float LookY = -0.062f;   ///< Base look direction Y.
    static constexpr float LookZ = -0.637f;   ///< Base look direction Z.
    static constexpr float SpinSpeed = 0.35f;   ///< Continuous 360 yaw speed (rad/s).
    static constexpr float PitchSwayDeg = 7.0f; ///< Up/down weightless bob amplitude, degrees.
    static constexpr float PitchSwaySpeed = 0.26f; ///< Speed of the pitch bob (rad/s).

    // --- Free-fly debug camera (F1 toggles; WASD move, Space/Shift up/down, arrows look) ---
    static constexpr int KeyFreeToggle = 290; ///< GLFW F1: toggle free-fly.
    static constexpr int KeyW = 87;           ///< Move forward.
    static constexpr int KeyA = 65;           ///< Strafe left.
    static constexpr int KeyS = 83;           ///< Move back.
    static constexpr int KeyD = 68;           ///< Strafe right.
    static constexpr int KeySpace = 32;       ///< Move up.
    static constexpr int KeyShiftL = 340;     ///< Move down.
    static constexpr int KeyLeft = 263;       ///< Look left.
    static constexpr int KeyRight = 262;      ///< Look right.
    static constexpr int KeyUp = 265;         ///< Look up.
    static constexpr int KeyDown = 264;       ///< Look down.
    static constexpr float FreeMoveSpeed = 0.4f;  ///< Move speed, in radius units per second.
    static constexpr float FreeLookSpeed = 70.0f; ///< Look speed, degrees per second.

    // --- "Fly into the portal" validation transition (tunable) ---
    static constexpr const char *PortalModelPath = "assets/transition/triangular/scene.gltf";
    static constexpr const char *PortalModelDir = "assets/transition/triangular";
    static constexpr float TransitionDuration = 3.4f; ///< Length of the transition, seconds.
    // First-person transition: walk from the cabin to the cockpit (facing forward, +Z), pause,
    // then accelerate through the windshield into the portal ahead. Offsets in radius units.
    static constexpr float WalkFraction = 0.30f;  ///< Progress at which the walk to the cockpit ends.
    static constexpr float PauseFraction = 0.16f; ///< Duration of the pause once at the cockpit.
    static constexpr float CockpitEyeX = 0.0f;    ///< Cockpit eye offset X from center.
    static constexpr float CockpitEyeY = 0.08f;   ///< Cockpit eye height.
    static constexpr float CockpitEyeZ = 0.30f;   ///< Cockpit eye Z (near the windshield).
    static constexpr float DiveDistance = 1.9f;   ///< Forward travel into the portal during the dive.
    static constexpr float PortalDistance = 1.35f;///< Portal distance ahead of the cockpit eye.
    static constexpr float PortalScale = 0.9f;    ///< Portal base size (grows over time).
    static constexpr float PortalSpinDeg = 220.0f;    ///< Portal spin across the whole transition, degrees.
    static constexpr float PortalYawDeg = 0.0f;       ///< Portal facing yaw offset (tune to face travel axis).
    static constexpr float PortalPitchDeg = 90.0f;    ///< Portal facing pitch offset (tune to face travel axis).
    static constexpr float FlashStart = 0.86f;        ///< Progress at which the white flash begins (0..1).

    Window &_window;                     ///< Borrowed window (owned by Core).
    GraphicsContext &_context;           ///< Borrowed graphics context (owned by Core).
    std::unique_ptr<AssetCache> _assets; ///< Owned asset cache (basic shader + quad).
    std::unique_ptr<TextRenderer> _text; ///< Owned text renderer for labels.
    std::unique_ptr<RenderModel> _ship;  ///< Dropship background model (nullptr if load failed).
    std::string _host;                   ///< Current host field value.
    std::string _port;                   ///< Current port field value (digits).
    RenderMode _mode;                    ///< Currently selected display mode.
    Field _focus;                        ///< Field with keyboard focus.
    std::string _error;                  ///< Last validation error (shown under the buttons).

    bool _freeFly;             ///< Free-fly debug camera active.
    Vec3 _freeEye;             ///< Free camera eye position (normalized model space).
    float _freeYaw;            ///< Free camera yaw, degrees (0 = +Z).
    float _freePitch;          ///< Free camera pitch, degrees.
    std::set<int> _held;       ///< Currently held movement/look keys (free-fly only).
    double _lastTime;          ///< Last frame time for free-fly dt.

    std::unique_ptr<RenderModel> _portal; ///< Transition portal model (nullptr if load failed).
    bool _transitioning;                  ///< True while the fly-into-portal animation plays.
    double _transitionStart;              ///< glfwGetTime() at which the transition began.
    MenuConfig _pendingConfig;            ///< Settings to return once the transition finishes.
};

} // namespace Zappy
