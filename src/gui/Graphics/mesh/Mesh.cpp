#include "Graphics/mesh/Mesh.hpp"
#include <cstddef>
#include <glad/glad.h>

#include "types/Vec.hpp"

namespace Zappy
{

namespace
{
// Per-instance model matrix is bound at fixed locations 8..11, leaving 0..7 for vertex
// attributes (pos, normal, uv, joints, weights, ...). Decoupling it from the vertex
// attribute count lets skinned (5 attrs) and static (3 attrs) meshes share one shader.
constexpr GLuint InstanceBaseLocation = 8;
} // namespace

Mesh::Mesh() : _vao(0), _vbo(0), _ebo(0), _instanceVbo(0), _indexCount(0), _attributeCount(0)
{
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_ebo);
    glGenBuffers(1, &_instanceVbo);
}

Mesh::~Mesh()
{
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ebo);
    glDeleteBuffers(1, &_instanceVbo);
}

void Mesh::upload(const std::vector<float> &vertices, const std::vector<unsigned int> &indices, const std::vector<unsigned int> &attributeSizes)
{
    if (indices.empty() || vertices.empty())
        throw MeshException("Mesh error: empty vertices or indices");
    if (attributeSizes.empty())
        throw MeshException("Mesh error: empty attribute layout");

    unsigned int stride = 0;

    for (unsigned int size : attributeSizes)
        stride += size;
    if (stride == 0 || vertices.size() % stride != 0)
        throw MeshException("Mesh error: vertex count is not a multiple of the attribute stride");

    _indexCount = static_cast<GLsizei>(indices.size());
    _attributeCount = static_cast<GLuint>(attributeSizes.size());
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    GLsizei strideBytes = static_cast<GLsizei>(stride * sizeof(float));
    std::size_t offset = 0;

    for (std::size_t location = 0; location < attributeSizes.size(); ++location)
    {
        glVertexAttribPointer(static_cast<GLuint>(location), static_cast<GLint>(attributeSizes[location]), GL_FLOAT, GL_FALSE, strideBytes, reinterpret_cast<const void *>(offset));
        glEnableVertexAttribArray(static_cast<GLuint>(location));
        offset += attributeSizes[location] * sizeof(float);
    }
    glBindVertexArray(0);
}

void Mesh::draw() const
{
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Mesh::drawInstanced(const std::vector<Mat4> &instances) const
{
    if (instances.empty())
        return;

    GLsizei columnStride = static_cast<GLsizei>(sizeof(Mat4));

    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _instanceVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(instances.size() * sizeof(Mat4)), instances.data(), GL_DYNAMIC_DRAW);
    for (GLuint column = 0; column < 4; ++column)
    {
        GLuint location = InstanceBaseLocation + column;
        std::size_t offset = column * sizeof(Vec4);

        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location, 4, GL_FLOAT, GL_FALSE, columnStride, reinterpret_cast<const void *>(offset));
        glVertexAttribDivisor(location, 1);
    }
    glDrawElementsInstanced(GL_TRIANGLES, _indexCount, GL_UNSIGNED_INT, nullptr, static_cast<GLsizei>(instances.size()));
    glBindVertexArray(0);
}

} // namespace Zappy
