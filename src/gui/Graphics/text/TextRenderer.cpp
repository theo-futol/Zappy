#include "Graphics/text/TextRenderer.hpp"

namespace Zappy
{

TextRenderer::TextRenderer() : _library(nullptr), _face(nullptr)
{
}

TextRenderer::~TextRenderer()
{
}

void TextRenderer::loadFont(const std::string &path, int pixelSize)
{
    (void)path;
    (void)pixelSize;
}

void TextRenderer::drawText(const std::string &text, float x, float y, Color color)
{
    (void)text;
    (void)x;
    (void)y;
    (void)color;
}

} // namespace Zappy
