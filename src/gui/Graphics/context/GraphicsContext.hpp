#pragma once

#include <exception>
#include <string>

namespace Zappy
{

/**
 * @class GraphicsContext
 * @brief Configures global OpenGL pipeline state (depth test, blending, viewport).
 */
class GraphicsContext
{
  public:
    /**
     * @class GraphicsContextException
     * @brief Error raised while configuring the OpenGL context.
     */
    class GraphicsContextException : public std::exception
    {
      public:
        explicit GraphicsContextException(const std::string &message) : _message(message)
        {
        }

        const char *what() const noexcept override
        {
            return _message.c_str();
        }

      private:
        std::string _message; ///< Error description.
    };

    GraphicsContext();

    /** @brief Enables the default render state (depth test, blending). */
    void configureDefaults();

    /**
     * @brief Sets the OpenGL viewport.
     * @param width Viewport width in pixels.
     * @param height Viewport height in pixels.
     */
    void setViewport(int width, int height);

    /**
     * @brief Enables or disables depth testing (disabled for 2D overlays like the HUD).
     * @param enabled True to enable depth testing.
     */
    void setDepthTest(bool enabled);
};

} // namespace Zappy
