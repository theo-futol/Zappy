#include "Graphics/cubemap/CubemapTexture.hpp"

#include <algorithm>
#include <cstddef>
#include <vector>

#include <lib/stb/stb_image.h>

namespace Zappy
{

CubemapTexture::CubemapTexture(const std::string &crossPath) : _id(0)
{
    int width = 0;
    int height = 0;
    int channels = 0;
    unsigned char *pixels = stbi_load(crossPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);

    if (pixels == nullptr)
        throw CubemapException("Cubemap error: cannot load image '" + crossPath + "'");

    int faceSize = width / 4;

    if (faceSize <= 0 || width != faceSize * 4 || height != faceSize * 3)
    {
        stbi_image_free(pixels);
        throw CubemapException("Cubemap error: '" + crossPath + "' is not a 4x3 cross");
    }
    glGenTextures(1, &_id);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _id);
    uploadFace(GL_TEXTURE_CUBE_MAP_POSITIVE_X, pixels, width, faceSize, 2, 1); // +X right
    uploadFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, pixels, width, faceSize, 0, 1); // -X left
    uploadFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, pixels, width, faceSize, 1, 0); // +Y top
    uploadFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, pixels, width, faceSize, 1, 2); // -Y bottom
    uploadFace(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, pixels, width, faceSize, 1, 1); // +Z front
    uploadFace(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, pixels, width, faceSize, 3, 1); // -Z back
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
    stbi_image_free(pixels);
}

CubemapTexture::~CubemapTexture()
{
    glDeleteTextures(1, &_id);
}

void CubemapTexture::bind(unsigned int slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_CUBE_MAP, _id);
}

void CubemapTexture::uploadFace(GLenum target, const unsigned char *cross, int crossWidth, int faceSize, int column, int row) const
{
    std::vector<unsigned char> face(static_cast<std::size_t>(faceSize) * static_cast<std::size_t>(faceSize) * 4);

    for (int y = 0; y < faceSize; ++y)
    {
        std::size_t sourceOffset = (static_cast<std::size_t>(row * faceSize + y) * static_cast<std::size_t>(crossWidth) + static_cast<std::size_t>(column * faceSize)) * 4;
        std::size_t destOffset = static_cast<std::size_t>(y) * static_cast<std::size_t>(faceSize) * 4;

        std::copy(cross + sourceOffset, cross + sourceOffset + static_cast<std::size_t>(faceSize) * 4, face.begin() + destOffset);
    }
    glTexImage2D(target, 0, GL_RGBA, faceSize, faceSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, face.data());
}

} // namespace Zappy
