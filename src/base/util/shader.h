#pragma once

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <string>

class Shader
{
public:
    unsigned int ID;

    Shader(const char* vertexPath, const char* fragmentPath);

    Shader(const char* vertexContent, const char* fragmentContent, bool fromString);

    void use();
    void setBool(const std::string& name, bool value) const;
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec3(const std::string& name, float x, float y, float z) const;
    void setVec2(const std::string& name, const glm::vec2& value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setMat4(const std::string& name, const glm::mat4& mat) const;
    void setMat3(const std::string& name, const glm::mat3& mat) const;

private:
    void compileShaders(const char* vShaderCode, const char* fShaderCode);
    void checkCompileErrors(unsigned int shader, std::string type);
};
