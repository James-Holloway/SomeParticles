#ifndef SHADER_HPP
#define SHADER_HPP

#include <memory>
#include <optional>
#include "GLM.hpp"
#include "SSBO.hpp"

enum class ShaderType
{
    Vertex,
    Fragment,
    Geometry,
    Compute,
};

unsigned int GetGLShaderType(ShaderType type);
std::optional<std::string> GetPathFromShaderName(const std::string &shaderName);


class Shader
{
public:
    explicit Shader(const std::string &shaderName, ShaderType shaderType);
    explicit Shader(ShaderType shaderType, const std::string &shaderString);
    ~Shader();
    
    unsigned int GLShader = 0;
    ShaderType ShaderType;

    explicit operator unsigned int() const;
};

class ShaderProgram
{
public:
    ShaderProgram(const std::shared_ptr<Shader> &vertexShader, const std::shared_ptr<Shader> &fragmentShader, const std::shared_ptr<Shader> &geometryShader = nullptr);
    explicit ShaderProgram(const std::shared_ptr<Shader> &computeShader);
    ~ShaderProgram();

    unsigned int GLProgram = 0;
    unsigned int GLVAO = 0;
    std::array<std::shared_ptr<SSBO>, 8> SSBOs;

    void Use() const;
    explicit operator unsigned int() const;

    static void Unbind();

    [[nodiscard]] int GetUniformLocation(const std::string &uniformName) const;
    void SetBool(const std::string &uniformName, bool value) const;
    void SetInt(const std::string &uniformName, int value) const;
    void SetFloat(const std::string &uniformName, float value) const;
    void SetMat4(const std::string &uniformName, const glm::mat4 &value) const;
    void SetMat3(const std::string &uniformName, const glm::mat3 &value) const;
    void SetMat2(const std::string &uniformName, const glm::mat2 &value) const;
    void SetVec4(const std::string &uniformName, glm::vec4 value) const;
    void SetVec3(const std::string &uniformName, glm::vec3 value) const;
    void SetVec2(const std::string &uniformName, glm::vec2 value) const;
    void SetIVec2(const std::string &uniformName, glm::ivec2 value) const;

    [[nodiscard]] int GetProgramSSBOBinding(const std::string& bufferName) const;
    void ClearSSBOs();
    void SetSSBO(int index, const std::shared_ptr<SSBO> &ssbo);
    void SetSSBO(const std::string& bufferName, const std::shared_ptr<SSBO> &ssbo);
};

#endif //SHADER_HPP
