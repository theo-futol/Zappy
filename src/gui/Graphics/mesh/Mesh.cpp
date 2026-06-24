#include "Graphics/mesh/Mesh.hpp"
#include <glad/glad.h>

namespace Zappy
{

Mesh::Mesh() : _vao(0), _vbo(0), _ebo(0), _indexCount(0)
{
    glGenVertexArrays(1, &_vao);
    glGenBuffers(1, &_vbo);
    glGenBuffers(1, &_ebo);
}

Mesh::~Mesh()
{
    glDeleteVertexArrays(1, &_vao);
    glDeleteBuffers(1, &_vbo);
    glDeleteBuffers(1, &_ebo);
}

void Mesh::upload(const std::vector<float> &vertices, const std::vector<unsigned int> &indices)
{
    if (indices.empty() || vertices.empty())
        throw MeshException("Mesh error: empty vertices or indices");
    _indexCount = static_cast<GLsizei>(indices.size());
    glBindVertexArray(_vao);
    glBindBuffer(GL_ARRAY_BUFFER, _vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
}

void Mesh::draw() const
{
    glBindVertexArray(_vao);
    glDrawElements(GL_TRIANGLES, _indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

} // namespace Zappy
