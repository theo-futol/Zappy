#pragma once

#include <exception>
#include <string>
#include <vector>

#include <glad/glad.h>

namespace Zappy
{

/**
 * @class Mesh
 * @brief Owns a vertex array and its buffers, and issues the draw call.
 */
class Mesh
{
  public:
    /**
     * @class MeshException
     * @brief Error raised while uploading mesh data.
     */
    class MeshException : public std::exception
    {
      public:
        explicit MeshException(const std::string &message) : _message(message)
        {
        }

        const char *what() const noexcept override
        {
            return _message.c_str();
        }

      private:
        std::string _message; ///< Error description.
    };

    Mesh();
    ~Mesh();

    Mesh(const Mesh &) = delete;
    Mesh &operator=(const Mesh &) = delete;
    Mesh(Mesh &&) = delete;
    Mesh &operator=(Mesh &&) = delete;

    /**
     * @brief Uploads vertex and index data to the GPU.
     * @param vertices Interleaved vertex attributes.
     * @param indices Element indices.
     */
    void upload(const std::vector<float> &vertices, const std::vector<unsigned int> &indices);

    /** @brief Draws the mesh with the currently bound shader. */
    void draw() const;

  private:
    GLuint _vao;         ///< Vertex array object.
    GLuint _vbo;         ///< Vertex buffer object.
    GLuint _ebo;         ///< Element buffer object.
    GLsizei _indexCount; ///< Number of indices to draw.
};

} // namespace Zappy
