#include "Graphics/text/TextRenderer.hpp"

#include <glm/ext/matrix_clip_space.hpp>

#include "types/Vec.hpp"

namespace Zappy
{

TextRenderer::TextRenderer() : _library(nullptr), _face(nullptr), _glyphs(), _shader(nullptr), _vao(0), _vbo(0), _width(0), _height(0)
{
    if (FT_Init_FreeType(&_library) != 0)
        throw TextRendererException("failed to initialize FreeType");
}

TextRenderer::~TextRenderer()
{
    for (std::pair<const char, Glyph> &entry : _glyphs)
        glDeleteTextures(1, &entry.second.textureId);
    if (_vbo != 0)
        glDeleteBuffers(1, &_vbo);
    if (_vao != 0)
        glDeleteVertexArrays(1, &_vao);
    if (_face != nullptr)
        FT_Done_Face(_face);
    if (_library != nullptr)
        FT_Done_FreeType(_library);
}

void TextRenderer::cacheGlyph(unsigned char character)
{
    if (FT_Load_Char(_face, character, FT_LOAD_RENDER) != 0)
        return;

    FT_GlyphSlot slot = _face->glyph;
    GLuint texture = 0;

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, static_cast<GLsizei>(slot->bitmap.width), static_cast<GLsizei>(slot->bitmap.rows), 0, GL_RED, GL_UNSIGNED_BYTE, slot->bitmap.buffer);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    _glyphs[static_cast<char>(character)] =
        Glyph{texture, static_cast<int>(slot->bitmap.width), static_cast<int>(slot->bitmap.rows), slot->bitmap_left, slot->bitmap_top, slot->advance.x};
}

void TextRenderer::loadFont(const std::string &path, int pixelSize)
{
    if (FT_New_Face(_library, path.c_str(), 0, &_face) != 0)
        throw TextRendererException("cannot load font: " + path);
    FT_Set_Pixel_Sizes(_face, 0, static_cast<FT_UInt>(pixelSize));

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (unsigned char character = 32; character < 127; ++character)
        cacheGlyph(character);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    try
    {
        _shader = std::make_unique<Shader>("shaders/text.vert", "shaders/text.frag");
    }
    catch (const Shader::ShaderException &error)
    {
        throw TextRendererException(std::string("text shader: ") + error.what());
    }

    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, nullptr, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    _shader->use();
    _shader->setUniform("uText", 0);
}

void TextRenderer::setScreenSize(int width, int height)
{
    _width = width;
    _height = height;
    if (_shader == nullptr)
        return;
    _shader->use();
    _shader->setUniform("uProjection", glm::ortho(0.0f, static_cast<float>(width), 0.0f, static_cast<float>(height)));
}

float TextRenderer::measure(const std::string &text, float scale) const
{
    float width = 0.0f;

    for (char character : text)
    {
        std::map<char, Glyph>::const_iterator found = _glyphs.find(character);

        if (found != _glyphs.end())
            width += static_cast<float>(found->second.advance >> 6) * scale;
    }
    return width;
}

void TextRenderer::drawText(const std::string &text, float x, float y, Color color, float scale)
{
    if (_shader == nullptr)
        return;

    _shader->use();
    _shader->setUniform("uTextColor", Vec3(color.r, color.g, color.b));
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);

    for (char character : text)
    {
        std::map<char, Glyph>::const_iterator found = _glyphs.find(character);

        if (found == _glyphs.end())
            continue;

        const Glyph &glyph = found->second;
        float xpos = x + static_cast<float>(glyph.bearingX) * scale;
        float ypos = y - static_cast<float>(glyph.height - glyph.bearingY) * scale;
        float w = static_cast<float>(glyph.width) * scale;
        float h = static_cast<float>(glyph.height) * scale;
        float vertices[6][4] = {
            {xpos, ypos + h, 0.0f, 0.0f}, {xpos, ypos, 0.0f, 1.0f},     {xpos + w, ypos, 1.0f, 1.0f},
            {xpos, ypos + h, 0.0f, 0.0f}, {xpos + w, ypos, 1.0f, 1.0f}, {xpos + w, ypos + h, 1.0f, 0.0f},
        };

        glBindTexture(GL_TEXTURE_2D, glyph.textureId);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        x += static_cast<float>(glyph.advance >> 6) * scale;
    }
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

} // namespace Zappy
