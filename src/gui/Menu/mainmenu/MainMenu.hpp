#pragma once

#include <exception>
#include <memory>
#include <string>

#include "Graphics/context/GraphicsContext.hpp"
#include "Graphics/text/TextRenderer.hpp"
#include "Graphics/window/Window.hpp"
#include "Render/asset/AssetCache.hpp"
#include "types/Color.hpp"
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

    /** @brief Draws one frame of the menu. */
    void draw();

    /**
     * @brief Draws a filled screen-space rectangle.
     * @param rect Rectangle to fill.
     * @param color Fill color.
     */
    void drawRect(const Rect &rect, const Vec3 &color);

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
    static constexpr float PanelHeight = 440.0f;  ///< Height of the centered menu panel.
    static constexpr float FieldWidth = 380.0f;   ///< Width of a text field / wide button.
    static constexpr float FieldHeight = 44.0f;   ///< Height of a text field.
    static constexpr float ButtonHeight = 48.0f;  ///< Height of an action button.
    static constexpr float RowGap = 78.0f;        ///< Vertical step between stacked rows.
    static constexpr float Pad = 40.0f;           ///< Inner padding of the panel.
    static constexpr int Backspace = 259;         ///< GLFW key code for Backspace.
    static constexpr int Enter = 257;             ///< GLFW key code for Enter.
    static constexpr int Tab = 258;               ///< GLFW key code for Tab.
    static constexpr int MaxPort = 65535;         ///< Highest valid TCP port.

    Window &_window;                     ///< Borrowed window (owned by Core).
    GraphicsContext &_context;           ///< Borrowed graphics context (owned by Core).
    std::unique_ptr<AssetCache> _assets; ///< Owned asset cache (basic shader + quad).
    std::unique_ptr<TextRenderer> _text; ///< Owned text renderer for labels.
    std::string _host;                   ///< Current host field value.
    std::string _port;                   ///< Current port field value (digits).
    RenderMode _mode;                    ///< Currently selected display mode.
    Field _focus;                        ///< Field with keyboard focus.
    std::string _error;                  ///< Last validation error (shown under the buttons).
};

} // namespace Zappy
