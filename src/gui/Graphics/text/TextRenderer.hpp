#pragma once

#include <exception>
#include <map>
#include <memory>
#include <string>

#include <glad/glad.h>

// clang-format off
#include <ft2build.h>
#include FT_FREETYPE_H
// clang-format on

#include "Graphics/shader/Shader.hpp"
#include "types/Color.hpp"

namespace Zappy
{

/**
 * @class TextRenderer
 * @brief Renders text using FreeType; the only place FreeType is touched.
 *
 * On load it rasterizes each printable ASCII glyph to a GL_RED texture and caches
 * it with its placement metrics. drawText then emits one textured quad per glyph
 * in screen space, sampling the glyph coverage as alpha.
 */
class TextRenderer
{
  public:
    /**
     * @class TextRendererException
     * @brief Error raised while initializing FreeType or loading a font.
     */
    class TextRendererException : public std::exception
    {
      public:
        explicit TextRendererException(const std::string &message) : _message(message)
        {
        }

        const char *what() const noexcept override
        {
            return _message.c_str();
        }

      private:
        std::string _message; ///< Error description.
    };

    TextRenderer();
    ~TextRenderer();

    TextRenderer(const TextRenderer &) = delete;
    TextRenderer &operator=(const TextRenderer &) = delete;
    TextRenderer(TextRenderer &&) = delete;
    TextRenderer &operator=(TextRenderer &&) = delete;

    /**
     * @brief Loads a font face, rasterizes its glyph atlas and builds the text shader.
     * @param path Path to the font file.
     * @param pixelSize Glyph pixel height.
     * @throws TextRendererException On a load or shader failure.
     */
    void loadFont(const std::string &path, int pixelSize);

    /**
     * @brief Sets the screen size used for the text projection (origin bottom-left).
     * @param width Window width in pixels.
     * @param height Window height in pixels.
     */
    void setScreenSize(int width, int height);

    /**
     * @brief Draws a string at a screen position (the pen starts on the baseline).
     * @param text Text to draw.
     * @param x Screen X position (left of the first glyph).
     * @param y Screen Y baseline position.
     * @param color Text color.
     */
    void drawText(const std::string &text, float x, float y, Color color);

  private:
    /**
     * @struct Glyph
     * @brief A cached rasterized character: its GPU texture and placement metrics.
     */
    struct Glyph
    {
        GLuint textureId; ///< GL_RED texture holding the glyph bitmap.
        int width;        ///< Bitmap width in pixels.
        int height;       ///< Bitmap height in pixels.
        int bearingX;     ///< Offset from the pen to the glyph's left edge.
        int bearingY;     ///< Height of the glyph above the baseline.
        long advance;     ///< Cursor advance to the next glyph, in 1/64 px.
    };

    /**
     * @brief Rasterizes one character and caches its texture + metrics.
     * @param character ASCII character to load; silently skipped on a load failure.
     */
    void cacheGlyph(unsigned char character);

    FT_Library _library;             ///< FreeType library handle.
    FT_Face _face;                   ///< Loaded font face.
    std::map<char, Glyph> _glyphs;   ///< Cached glyph atlas, one entry per character.
    std::unique_ptr<Shader> _shader; ///< Screen-space text shader (GL_RED sampling).
    GLuint _vao;                     ///< Vertex array for the per-glyph quad.
    GLuint _vbo;                     ///< Dynamic vertex buffer, rewritten per glyph.
    int _width;                      ///< Screen width in pixels.
    int _height;                     ///< Screen height in pixels.
};

} // namespace Zappy
