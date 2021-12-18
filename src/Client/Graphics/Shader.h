//
// Created by lukas on 18.12.21.
//

#ifndef MINECRAFT_CLONE_SHADER_H
#define MINECRAFT_CLONE_SHADER_H

#include <unordered_map>
#include <glad/glad.h>
#include <string>
#include <glm/glm.hpp>

class Shader {
public:
    Shader(const std::string& filepath);
    Shader(const std::string& vertexSrc, const std::string& fragmentSrc);
    ~Shader();
    void Bind() const;
    void Unbind() const;

    void SetInt(const std::string& name, int value);
    void SetUInt(const std::string& name, unsigned int value);
    void SetIntArray(const std::string& name, int* values, uint32_t count);
    void SetFloat(const std::string& name, float value);
    void SetFloat2(const std::string& name, const glm::vec2& value);
    void SetFloat3(const std::string& name, const glm::vec3& value);
    void SetFloat4(const std::string& name, const glm::vec4& value);
    void SetMat3(const std::string& name, const glm::mat3& value);
    void SetMat4(const std::string& name, const glm::mat4& value);
private:
    std::string ReadFile(const std::string& filepath);
    std::unordered_map<GLenum, std::string> PreProcess(const std::string& source);

    void Compile(std::unordered_map<GLenum, std::string>& shaderSources);
private:
    uint32_t m_RendererID;
};


#endif
