#pragma once

#include <exception>
#include <string>

#include <glad/glad.h>

namespace Zappy
{

/**
 * @class Texture
 * @brief Loads an image into an OpenGL texture; the only place stb_image is used.
 */
class Texture
{
  public:
    /**
     * @class TextureException
     * @brief Error raised while loading an image.
     */
    class TextureException : public std::exception
    {
      public:
        explicit TextureException(const std::string &message) : _message(message)
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
     * @brief Loads a texture from an image file.
     * @param path Path to the image.
     * @throws TextureException On a load or decode failure.
     */
    explicit Texture(const std::string &path);

    ~Texture();

    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;
    Texture(Texture &&) = delete;
    Texture &operator=(Texture &&) = delete;

    /** @brief Binds the texture to a texture unit. @param slot Texture unit index. */
    void bind(unsigned int slot) const;

    /** @brief Texture width. @return The width in pixels. */
    int width() const;

    /** @brief Texture height. @return The height in pixels. */
    int height() const;

  private:
    GLuint _id;  ///< OpenGL texture object.
    int _width;  ///< Width in pixels.
    int _height; ///< Height in pixels.
};

} // namespace Zappy
