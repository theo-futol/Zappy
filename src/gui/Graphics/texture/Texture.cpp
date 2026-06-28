#include "Graphics/texture/Texture.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include <lib/stb/stb_image.h>

namespace Zappy
{

Texture::Texture(const std::string &path) : _id(0), _width(0), _height(0)
{
    int channels = 0;
    unsigned char *pixels = stbi_load(path.c_str(), &_width, &_height, &channels, STBI_rgb_alpha);

    if (pixels == nullptr)
        throw TextureException("Texture error: cannot load image '" + path + "'");
    glGenTextures(1, &_id);
    glBindTexture(GL_TEXTURE_2D, _id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    stbi_image_free(pixels);
}

Texture::~Texture()
{
    glDeleteTextures(1, &_id);
}

void Texture::bind(unsigned int slot) const
{
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, _id);
}

int Texture::width() const
{
    return _width;
}

int Texture::height() const
{
    return _height;
}

} // namespace Zappy
