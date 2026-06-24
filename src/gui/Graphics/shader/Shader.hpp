#pragma once

#include <exception>
#include <string>

#include <glad/glad.h>

#include "types/Mat.hpp"
#include "types/Vec.hpp"

namespace Zappy
{

/**
 * @class Shader
 * @brief Compiles, links and binds a GLSL shader program.
 */
class Shader
{
  public:
    /**
     * @class ShaderException
     * @brief Error raised during shader compilation or linking.
     */
    class ShaderException : public std::exception
    {
      public:
        explicit ShaderException(const std::string &message) : _message(message)
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
     * @brief Builds the program from GLSL source files.
     * @param vertexPath Path to the vertex shader source.
     * @param fragmentPath Path to the fragment shader source.
     * @throws ShaderException On a compilation or linking error.
     */
    Shader(const std::string &vertexPath, const std::string &fragmentPath);

    ~Shader();

    Shader(const Shader &) = delete;
    Shader &operator=(const Shader &) = delete;
    Shader(Shader &&) = delete;
    Shader &operator=(Shader &&) = delete;

    /** @brief Binds the program for subsequent draw calls. */
    void use() const;

    /** @brief Sets a mat4 uniform. @param name Uniform name. @param value Matrix value. */
    void setUniform(const std::string &name, const Mat4 &value);

    /** @brief Sets a vec3 uniform. @param name Uniform name. @param value Vector value. */
    void setUniform(const std::string &name, const Vec3 &value);

    /** @brief Sets a float uniform. @param name Uniform name. @param value Float value. */
    void setUniform(const std::string &name, float value);

    /** @brief Sets an int uniform. @param name Uniform name. @param value Int value. */
    void setUniform(const std::string &name, int value);

  private:
    /**
     * @brief Links a vertex and a fragment shader into a program.
     * @param vertexShader Compiled vertex shader id.
     * @param fragmentShader Compiled fragment shader id.
     * @return The linked program id.
     * @throws ShaderException On a linking error.
     */
    GLuint createProgram(GLuint vertexShader, GLuint fragmentShader);

    /**
     * @brief Reads a GLSL source file into a string.
     * @param path Path to the source file.
     * @return The file contents.
     * @throws ShaderException If the file cannot be opened.
     */
    std::string readFile(const std::string &path) const;

    /**
     * @brief Compiles a single shader stage from its source.
     * @param source GLSL source code.
     * @param type Shader stage (GL_VERTEX_SHADER or GL_FRAGMENT_SHADER).
     * @return The compiled shader id.
     * @throws ShaderException On a compilation error.
     */
    GLuint compile(const std::string &source, GLenum type);

    /**
     * @brief Resolves a uniform location, warning (without throwing) if it is missing.
     * @param name Uniform name.
     * @return The location, or -1 if the uniform does not exist or was optimized out.
     */
    GLint uniformLocation(const std::string &name) const;

    GLuint _program; ///< OpenGL program object, 0 when unset.
};

} // namespace Zappy
