#include "Graphics/window/Window.hpp"

namespace Zappy
{

Window::Window(int width, int height, const std::string &title) : _window(nullptr), _width(width), _height(height), _events()
{
    if (glfwInit() == GLFW_FALSE)
        throw WindowException("Failed to initialize GLFW");

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    _window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);
    if (_window == nullptr)
    {
        glfwTerminate();
        throw WindowException("Failed to create the GLFW window");
    }

    glfwMakeContextCurrent(_window);
    if (gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress)) == 0)
    {
        glfwDestroyWindow(_window);
        glfwTerminate();
        throw WindowException("Failed to load OpenGL through GLAD");
    }

    glfwSetWindowUserPointer(_window, this);
    registerCallbacks();
}

Window::~Window()
{
    if (_window != nullptr)
        glfwDestroyWindow(_window);
    glfwTerminate();
}

void Window::registerCallbacks()
{
    glfwSetWindowCloseCallback(_window, closeCallback);
    glfwSetKeyCallback(_window, keyCallback);
    glfwSetFramebufferSizeCallback(_window, framebufferSizeCallback);
}

Window *Window::fromGlfw(GLFWwindow *window)
{
    return static_cast<Window *>(glfwGetWindowUserPointer(window));
}

void Window::closeCallback(GLFWwindow *window)
{
    Event event{};

    event.type = EventType::Close;
    fromGlfw(window)->_events.push_back(event);
}

void Window::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    Event event{};

    (void)scancode;
    (void)mods;
    if (key != GLFW_KEY_ESCAPE || action != GLFW_PRESS)
        return;
    event.type = EventType::Close;
    fromGlfw(window)->_events.push_back(event);
}

void Window::framebufferSizeCallback(GLFWwindow *window, int width, int height)
{
    Window *self = fromGlfw(window);
    Event event{};

    self->_width = width;
    self->_height = height;
    event.type = EventType::Resize;
    event.width = width;
    event.height = height;
    self->_events.push_back(event);
}

bool Window::isOpen() const
{
    return glfwWindowShouldClose(_window) == GLFW_FALSE;
}

void Window::clear(Color color)
{
    glClearColor(color.r, color.g, color.b, color.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Window::swapBuffers()
{
    glfwSwapBuffers(_window);
}

std::vector<Event> Window::pollEvents()
{
    _events.clear();
    glfwPollEvents();
    return _events;
}

int Window::width() const
{
    return _width;
}

int Window::height() const
{
    return _height;
}

} // namespace Zappy
