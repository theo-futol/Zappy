#pragma once

#include <exception>
#include <string>
#include <vector>

#include <glad/glad.h>

#include "types/Mat.hpp"

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
     * @brief Uploads interleaved vertex and index data to the GPU.
     * @param vertices Interleaved vertex attributes (all attributes of one vertex, then the next).
     * @param indices Element indices.
     * @param attributeSizes Component count of each attribute, in layout order
     *        (e.g. {3} for position-only, {3, 3, 2} for position + normal + uv).
     *        Attribute i is bound to shader location i.
     * @throws MeshException On empty data, empty layout, or a vertex count that
     *         is not a whole multiple of the per-vertex stride.
     */
    void upload(const std::vector<float> &vertices, const std::vector<unsigned int> &indices, const std::vector<unsigned int> &attributeSizes);

    /** @brief Draws the mesh once with the currently bound shader. */
    void draw() const;

    /**
     * @brief Draws the mesh many times in one call, one copy per instance matrix.
     *
     * Uploads the per-instance model matrices to a dedicated buffer wired as
     * vertex attributes (a mat4 occupying the four locations right after the
     * vertex attributes), each advancing once per instance, then issues a single
     * glDrawElementsInstanced. The bound shader must read that instance matrix.
     * @param instances Model matrix of each instance; nothing is drawn if empty.
     */
    void drawInstanced(const std::vector<Mat4> &instances) const;

  private:
    GLuint _vao;               ///< Vertex array object.
    GLuint _vbo;               ///< Vertex buffer object.
    GLuint _ebo;               ///< Element buffer object.
    GLuint _instanceVbo;       ///< Per-instance model-matrix buffer (for instanced draws).
    GLsizei _indexCount;       ///< Number of indices to draw.
    GLuint _attributeCount;    ///< Number of vertex attributes (first free instance location).
};

} // namespace Zappy
