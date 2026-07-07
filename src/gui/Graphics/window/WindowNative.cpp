#include "Graphics/window/Window.hpp"

// clang-format off
// X11/Xlib.h defines macros (KeyPress, KeyRelease, None, Status, Bool, True, False, ...)
// that collide with identifiers used throughout the rest of the codebase, so the native
// accessors live alone in this translation unit and touch nothing else.
#define GLFW_EXPOSE_NATIVE_X11
#define GLFW_EXPOSE_NATIVE_GLX
#include <GLFW/glfw3native.h>
// clang-format on

namespace Zappy
{

void *Window::nativeDisplay() const
{
    return glfwGetX11Display();
}

void *Window::nativeGLXContext() const
{
    return glfwGetGLXContext(_window);
}

unsigned long Window::nativeGLXWindow() const
{
    return glfwGetGLXWindow(_window);
}

} // namespace Zappy
