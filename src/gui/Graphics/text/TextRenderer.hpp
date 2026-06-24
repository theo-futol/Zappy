#pragma once

#include <exception>
#include <string>

// clang-format off
#include <ft2build.h>
#include FT_FREETYPE_H
// clang-format on

#include "types/Color.hpp"

namespace Zappy
{

/**
 * @class TextRenderer
 * @brief Renders text using FreeType; the only place FreeType is touched.
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
     * @brief Loads a font face at a given pixel size.
     * @param path Path to the font file.
     * @param pixelSize Glyph pixel height.
     * @throws TextRendererException On a load failure.
     */
    void loadFont(const std::string &path, int pixelSize);

    /**
     * @brief Draws a string at a screen position.
     * @param text Text to draw.
     * @param x Screen X position.
     * @param y Screen Y position.
     * @param color Text color.
     */
    void drawText(const std::string &text, float x, float y, Color color);

  private:
    FT_Library _library; ///< FreeType library handle.
    FT_Face _face;       ///< Loaded font face.
};

} // namespace Zappy
