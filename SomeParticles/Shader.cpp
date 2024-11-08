#include "Shader.hpp"

#include <fstream>
#include <filesystem>
#include <vector>
#include <glad/glad.h>

unsigned int GetGLShaderType(const ShaderType type)
{
    switch (type)
    {
        case ShaderType::Vertex:
            return GL_VERTEX_SHADER;
        case ShaderType::Fragment:
            return GL_FRAGMENT_SHADER;
        case ShaderType::Geometry:
            return GL_GEOMETRY_SHADER;
        case ShaderType::Compute:
            return GL_COMPUTE_SHADER;
        default:
            return 0;
    }
}

std::optional<std::string> GetPathFromShaderName(const std::string &shaderName)
{
    namespace fs = std::filesystem;

    const auto path = fs::current_path().string();

    // CWD/shaderName
    fs::path shaderPath = path + "/" + shaderName;
    if (fs::exists(shaderPath))
    {
        return {shaderPath};
    }

    // CWD/Shaders/shaderName
    shaderPath = path + "/Shaders/" + shaderName;
    if (fs::exists(shaderPath))
    {
        return {shaderPath};
    }

    // shaderName as path
    if (fs::exists(shaderName))
    {
        return {shaderName};
    }

    return {};
}

Shader::Shader(const std::string &shaderName, ::ShaderType shaderType) : ShaderName(shaderName), ShaderType(shaderType)
{
    auto shaderPath = GetPathFromShaderName(shaderName);
    if (!shaderPath.has_value())
    {
        throw std::runtime_error("Failed to load shader from file " + shaderName);
    }

    std::ifstream shaderFile{shaderPath.value(), std::ios::ate | std::ios::in};
    if (!shaderFile.is_open())
    {
        throw std::runtime_error("Failed to open shader file " + shaderName);;
    }
    auto fileSize = shaderFile.tellg();
    std::vector<char> shaderSource;
    shaderSource.resize(static_cast<size_t>(fileSize) + 1);
    shaderFile.seekg(0);
    shaderFile.read(shaderSource.data(), fileSize);
    shaderFile.close();
    shaderSource.push_back('\0');

    GLShader = glCreateShader(GetGLShaderType(shaderType));

    const char *shaderSourcePointer = shaderSource.data();
    glShaderSource(GLShader, 1, &shaderSourcePointer, nullptr);
    glCompileShader(GLShader);

    int success;
    glGetShaderiv(GLShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(GLShader, 512, nullptr, infoLog);
        throw std::runtime_error("Failed to compile shader \"" + shaderName + "\": " + infoLog);
    }
}

Shader::~Shader()
{
    if (GLShader != 0)
    {
        glDeleteShader(GLShader);
    }
}

Shader::operator unsigned int() const
{
    return GLShader;
}

ShaderProgram::ShaderProgram(const std::shared_ptr<Shader> &vertexShader, const std::shared_ptr<Shader> &fragmentShader, const std::shared_ptr<Shader> &geometryShader)
{
    // Program
    GLProgram = glCreateProgram();
    glAttachShader(GLProgram, vertexShader->GLShader);
    glAttachShader(GLProgram, fragmentShader->GLShader);
    if (geometryShader != nullptr)
    {
        glAttachShader(GLProgram, geometryShader->GLShader);
    }

    glLinkProgram(GLProgram);

    glDetachShader(GLProgram, vertexShader->GLShader);
    glDetachShader(GLProgram, fragmentShader->GLShader);
    if (geometryShader != nullptr)
    {
        glDetachShader(GLProgram, geometryShader->GLShader);
    }

    int success;
    glGetProgramiv(GLProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(GLProgram, 512, nullptr, infoLog);
        throw std::runtime_error(std::string("Failed to link shader program: ") + infoLog);
    }

    // VAO
    glGenVertexArrays(1, &GLVAO);
    glBindVertexArray(GLVAO);
    glEnableVertexAttribArray(0);
}

ShaderProgram::ShaderProgram(const std::shared_ptr<Shader> &computeShader)
{
    // Program
    GLProgram = glCreateProgram();
    glAttachShader(GLProgram, computeShader->GLShader);

    glLinkProgram(GLProgram);

    glDetachShader(GLProgram, computeShader->GLShader);

    int success;
    glGetProgramiv(GLProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        char infoLog[512];
        glGetShaderInfoLog(GLProgram, 512, nullptr, infoLog);
        throw std::runtime_error(std::string("Failed to link shader program: ") + infoLog);
    }
}

ShaderProgram::~ShaderProgram()
{
    if (GLProgram != 0)
    {
        glDeleteProgram(GLProgram);
    }
}

void ShaderProgram::Use() const
{
    glUseProgram(GLProgram);
    if (GLVAO != 0)
    {
        glBindVertexArray(GLVAO);
    }

    for (int i = 0; i < SSBOs.size(); i++)
    {
        const auto &ssbo = SSBOs[i];
        if (ssbo == nullptr)
        {
            SSBO::Unbind();
            continue;
        }

        ssbo->Bind();
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, ssbo->GLBuffer);
    }

    SSBO::Unbind();
}

ShaderProgram::operator unsigned int() const
{
    return GLProgram;
}

void ShaderProgram::Unbind()
{
    glUseProgram(0);
}

int ShaderProgram::GetUniformLocation(const std::string &uniformName) const
{
    return glGetUniformLocation(GLProgram, uniformName.c_str());
}

void ShaderProgram::SetBool(const std::string &uniformName, const bool value) const
{
    auto location = GetUniformLocation(uniformName);
    if (location == -1)
        return;

    glProgramUniform1i(GLProgram, location, value ? 1 : 0);
}

void ShaderProgram::SetInt(const std::string &uniformName, const int value) const
{
    auto location = GetUniformLocation(uniformName);
    if (location == -1)
        return;

    glProgramUniform1i(GLProgram, location, value);
}

void ShaderProgram::SetFloat(const std::string &uniformName, const float value) const
{
    auto location = GetUniformLocation(uniformName);
    if (location == -1)
        return;

    glProgramUniform1f(GLProgram, location, value);
}

void ShaderProgram::SetMat4(const std::string &uniformName, const glm::mat4 &value) const
{
    auto location = GetUniformLocation(uniformName);
    if (location == -1)
        return;

    glProgramUniformMatrix4fv(GLProgram, location, 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderProgram::SetMat3(const std::string &uniformName, const glm::mat3 &value) const
{
    auto location = GetUniformLocation(uniformName);
    if (location == -1)
        return;

    glProgramUniformMatrix3fv(GLProgram, location, 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderProgram::SetMat2(const std::string &uniformName, const glm::mat2 &value) const
{
    auto location = GetUniformLocation(uniformName);
    if (location == -1)
        return;

    glProgramUniformMatrix2fv(GLProgram, location, 1, GL_FALSE, glm::value_ptr(value));
}

void ShaderProgram::SetVec4(const std::string &uniformName, glm::vec4 value) const
{
    auto location = GetUniformLocation(uniformName);
    if (location == -1)
        return;

    glProgramUniform4fv(GLProgram, location, 1, glm::value_ptr(value));
}

void ShaderProgram::SetVec3(const std::string &uniformName, glm::vec3 value) const
{
    auto location = GetUniformLocation(uniformName);
    if (location == -1)
        return;

    glProgramUniform3fv(GLProgram, location, 1, glm::value_ptr(value));
}

void ShaderProgram::SetVec2(const std::string &uniformName, glm::vec2 value) const
{
    auto location = GetUniformLocation(uniformName);
    if (location == -1)
        return;

    glProgramUniform2fv(GLProgram, location, 1, glm::value_ptr(value));
}

void ShaderProgram::SetIVec2(const std::string &uniformName, glm::ivec2 value) const
{
    auto location = GetUniformLocation(uniformName);
    if (location == -1)
        return;

    glProgramUniform2iv(GLProgram, location, 1, glm::value_ptr(value));
}

int ShaderProgram::GetProgramSSBOBinding(const std::string &bufferName) const
{
    int index = glGetProgramResourceIndex(GLProgram, GL_SHADER_STORAGE_BLOCK, bufferName.c_str());
    if (index == -1)
    {
        return -1;
    }

    constexpr GLenum prop = GL_BUFFER_BINDING;
    int length;
    int item = -1;
    glGetProgramResourceiv(GLProgram, GL_SHADER_STORAGE_BLOCK, index, 1, &prop, 1, &length, &item);
    return item;
}

void ShaderProgram::ClearSSBOs()
{
    for (auto &ssbo : SSBOs)
    {
        ssbo = nullptr;
    }
}

void ShaderProgram::SetSSBO(const int index, const std::shared_ptr<SSBO> &ssbo)
{
    SSBOs.at(index) = ssbo;
}

void ShaderProgram::SetSSBO(const std::string &bufferName, const std::shared_ptr<SSBO> &ssbo)
{
    auto index = GetProgramSSBOBinding(bufferName);
    if (index < 0 || index >= SSBOs.size())
    {
        return;
    }

    SSBOs.at(index) = ssbo;
}
