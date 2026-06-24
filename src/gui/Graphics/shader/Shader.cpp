#include "Graphics/shader/Shader.hpp"
#include <fstream>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <sstream>

namespace Zappy
{

Shader::Shader(const std::string &vertexPath, const std::string &fragmentPath) : _program(0)
{
    GLuint vertexShader = 0;
    GLuint fragmentShader = 0;
    try
    {
        std::string vertexShaderSource = readFile(vertexPath);
        std::string fragmentShaderSource = readFile(fragmentPath);
        vertexShader = compile(vertexShaderSource, GL_VERTEX_SHADER);
        fragmentShader = compile(fragmentShaderSource, GL_FRAGMENT_SHADER);
        _program = createProgram(vertexShader, fragmentShader);
    }
    catch (const ShaderException &e)
    {
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
        throw ShaderException("Shader error: " + std::string(e.what()));
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

Shader::~Shader()
{
    if (_program)
        glDeleteProgram(_program);
}

GLuint Shader::createProgram(GLuint vertexShader, GLuint fragmentShader)
{
    GLuint program = glCreateProgram();
    GLint verifier = 0;
    GLint logLength = 0;

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &verifier);
    if (!verifier)
    {
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(static_cast<std::size_t>(logLength), '\0');
        glGetProgramInfoLog(program, logLength, nullptr, log.data());
        glDeleteProgram(program);
        throw ShaderException(log);
    }
    return program;
}

std::string Shader::readFile(const std::string &path) const
{
    std::stringstream iss;

    std::ifstream file(path);
    if (!file.is_open())
        throw ShaderException("Cannot open shader File: " + path + ".");
    iss << file.rdbuf();
    return iss.str();
}

GLuint Shader::compile(const std::string &source, GLenum type)
{
    GLuint shader = glCreateShader(type);
    const char *sSource = source.c_str();
    GLint verifier = 0;
    GLint logLength = 0;

    glShaderSource(shader, 1, &sSource, nullptr);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &verifier);
    if (!verifier)
    {
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::string log(static_cast<std::size_t>(logLength), '\0');
        glGetShaderInfoLog(shader, logLength, nullptr, log.data());
        glDeleteShader(shader);
        throw ShaderException(log);
    }
    return shader;
}

void Shader::use() const
{
    glUseProgram(_program);
}

GLint Shader::uniformLocation(const std::string &name) const
{
    GLint location = glGetUniformLocation(_program, name.c_str());

    if (location == -1)
        std::cerr << "[Shader] uniform not found: " << name << std::endl;
    return location;
}

void Shader::setUniform(const std::string &name, const Mat4 &value)
{
    glUniformMatrix4fv(uniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

void Shader::setUniform(const std::string &name, const Vec3 &value)
{
    glUniform3fv(uniformLocation(name), 1, glm::value_ptr(value));
}

void Shader::setUniform(const std::string &name, float value)
{
    glUniform1f(uniformLocation(name), value);
}

void Shader::setUniform(const std::string &name, int value)
{
    glUniform1i(uniformLocation(name), value);
}

} // namespace Zappy
