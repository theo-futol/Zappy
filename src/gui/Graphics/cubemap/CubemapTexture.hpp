#pragma once

#include <exception>
#include <string>

#include <glad/glad.h>

namespace Zappy
{

/**
 * @class CubemapTexture
 * @brief A GPU cube-map texture built from a single 4x3 horizontal-cross image.
 *
 * The source image packs the six faces in the classic cross layout
 * (columns x rows of equal square tiles): +Y on top, the -X/+Z/+X/-Z band in the
 * middle, and -Y at the bottom. The six tiles are cut out and uploaded to the
 * matching cube-map faces, ready to be sampled by a skybox shader.
 */
class CubemapTexture
{
  public:
    /**
     * @class CubemapException
     * @brief Error raised while loading or cutting the cross image.
     */
    class CubemapException : public std::exception
    {
      public:
        explicit CubemapException(const std::string &message) : _message(message)
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
     * @brief Loads a 4x3 cross image and uploads its six faces to a cube map.
     * @param crossPath Path to the cross image (width must be 4 tiles, height 3 tiles).
     * @throws CubemapException If the image cannot be read or is not a 4x3 cross.
     */
    explicit CubemapTexture(const std::string &crossPath);
    ~CubemapTexture();

    CubemapTexture(const CubemapTexture &) = delete;
    CubemapTexture &operator=(const CubemapTexture &) = delete;
    CubemapTexture(CubemapTexture &&) = delete;
    CubemapTexture &operator=(CubemapTexture &&) = delete;

    /**
     * @brief Binds the cube map to a texture unit.
     * @param slot Texture unit index (0 binds GL_TEXTURE0).
     */
    void bind(unsigned int slot) const;

  private:
    /**
     * @brief Cuts one square tile out of the cross and uploads it to a cube-map face.
     * @param target Cube-map face target (e.g. GL_TEXTURE_CUBE_MAP_POSITIVE_X).
     * @param cross RGBA pixels of the whole cross image.
     * @param crossWidth Width of the cross image in pixels.
     * @param faceSize Side length of one face tile in pixels.
     * @param column Tile column of the face in the cross.
     * @param row Tile row of the face in the cross.
     */
    void uploadFace(GLenum target, const unsigned char *cross, int crossWidth, int faceSize, int column, int row) const;

    GLuint _id; ///< Cube-map texture object.
};

} // namespace Zappy
