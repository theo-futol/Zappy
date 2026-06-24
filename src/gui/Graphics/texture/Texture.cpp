#include "Graphics/texture/Texture.hpp"

namespace Zappy
{

Texture::Texture(const std::string &path) : _id(0), _width(0), _height(0)
{
    (void)path;
}

Texture::~Texture()
{
}

void Texture::bind(unsigned int slot) const
{
    (void)slot;
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
