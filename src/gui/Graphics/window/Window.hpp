#pragma once

#include <exception>
#include <string>
#include <vector>

// clang-format off
#include <glad/glad.h>
#include <GLFW/glfw3.h>
// clang-format on

#include "types/Color.hpp"
#include "types/Event.hpp"

namespace Zappy
{

/**
 * @class Window
 * @brief Encapsulates the GLFW window and OpenGL context; the only place GLFW is touched
 * (aside from the native-handle escape hatch used by OpenXR/GLX interop, see nativeDisplay()).
 */
class Window
{
  public:
    /**
     * @class WindowException
     * @brief Error raised during window or context creation.
     */
    class WindowException : public std::exception
    {
      public:
        explicit WindowException(const std::string &message) : _message(message)
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
     * @brief Creates the window and the OpenGL context.
     * @param width Window width in pixels.
     * @param height Window height in pixels.
     * @param title Window title.
     * @throws WindowException On GLFW, window or loader failure.
     */
    Window(int width, int height, const std::string &title);

    ~Window();

    Window(const Window &) = delete;
    Window &operator=(const Window &) = delete;
    Window(Window &&) = delete;
    Window &operator=(Window &&) = delete;

    /** @brief Whether the window is still open. @return True while open. */
    bool isOpen() const;

    /** @brief Clears the framebuffer. @param color Clear color. */
    void clear(Color color);

    /** @brief Swaps the front and back buffers. */
    void swapBuffers();

    /** @brief Polls and translates pending input events. @return The events for this frame. */
    std::vector<Event> pollEvents();

    /** @brief Window width. @return The width in pixels. */
    int width() const;

    /** @brief Window height. @return The height in pixels. */
    int height() const;

    /**
     * @brief Native X11 display connection backing this window.
     *
     * Escape hatch for OpenXR's OpenGL/GLX graphics binding (XR_KHR_opengl_enable on Linux);
     * the only reason Window exposes a native handle beyond GLFWwindow*. Returned as an opaque
     * pointer (actually an X11 `Display*`) so this header never has to include X11/GLX headers.
     * @return The native X11 display handle.
     */
    void *nativeDisplay() const;

    /**
     * @brief Native GLX rendering context bound to this window's OpenGL context.
     * @return The native GLX context handle (actually a `GLXContext`), opaque here.
     */
    void *nativeGLXContext() const;

    /**
     * @brief Native GLX drawable backing this window.
     * @return The native GLX window handle (actually a `GLXWindow`, an X11 XID).
     */
    unsigned long nativeGLXWindow() const;

  private:
    /** @brief Registers the GLFW callbacks that feed the event queue. */
    void registerCallbacks();

    /**
     * @brief Retrieves the Window instance bound to a GLFW window.
     * @param window Native GLFW window.
     * @return The owning Window.
     */
    static Window *fromGlfw(GLFWwindow *window);

    /** @brief GLFW close callback; pushes a Close event. @param window Native window. */
    static void closeCallback(GLFWwindow *window);

    /**
     * @brief GLFW key callback; maps Escape to a Close event.
     * @param window Native window.
     * @param key GLFW key code.
     * @param scancode Platform scancode (unused).
     * @param action Press/release/repeat.
     * @param mods Modifier bitfield (unused).
     */
    static void keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods);

    /**
     * @brief GLFW character callback; pushes a Char event carrying the Unicode codepoint.
     * @param window Native window.
     * @param codepoint Unicode codepoint of the typed character.
     */
    static void charCallback(GLFWwindow *window, unsigned int codepoint);

    /**
     * @brief GLFW framebuffer-resize callback; pushes a Resize event.
     * @param window Native window.
     * @param width New framebuffer width.
     * @param height New framebuffer height.
     */
    static void framebufferSizeCallback(GLFWwindow *window, int width, int height);

    /**
     * @brief GLFW scroll callback; pushes a Scroll event.
     * @param window Native window.
     * @param xoffset Horizontal scroll (unused).
     * @param yoffset Vertical scroll amount.
     */
    static void scrollCallback(GLFWwindow *window, double xoffset, double yoffset);

    /**
     * @brief GLFW mouse-button callback; pushes a MouseButton event with the cursor position.
     * @param window Native window.
     * @param button GLFW button code.
     * @param action Press or release.
     * @param mods Modifier bitfield (unused).
     */
    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);

    /**
     * @brief GLFW cursor-position callback; pushes a MouseMove event.
     * @param window Native window.
     * @param xpos Cursor X position.
     * @param ypos Cursor Y position.
     */
    static void cursorPosCallback(GLFWwindow *window, double xpos, double ypos);

    GLFWwindow *_window;        ///< Native GLFW window handle.
    int _width;                 ///< Window width in pixels.
    int _height;                ///< Window height in pixels.
    std::vector<Event> _events; ///< Events collected during the current poll.
};

} // namespace Zappy
