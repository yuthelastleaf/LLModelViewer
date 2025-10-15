#ifndef SHADER_H
#define SHADER_H

#include <QOpenGLFunctions_3_3_Core>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>

class Shader : protected QOpenGLFunctions_3_3_Core
{
public:
    unsigned int ID;
    
    // 从文件路径构造
    Shader(const char* vertexPath, const char* fragmentPath);
    
    // 从字符串内容构造
    Shader(const char* vertexContent, const char* fragmentContent, bool fromString);
    
    ~Shader();
    
    // 使用/激活着色器程序
    void use();
    
    // uniform 工具函数 - 移除 const
    void setBool(const std::string& name, bool value);        // 移除 const
    void setInt(const std::string& name, int value);          // 移除 const
    void setFloat(const std::string& name, float value);      // 移除 const
    void setVec2(const std::string& name, const glm::vec2& value);   // 移除 const
    void setVec3(const std::string& name, float x, float y, float z);// 移除 const
    void setVec3(const std::string& name, const glm::vec3& value);   // 移除 const
    void setVec4(const std::string& name, const glm::vec4& value);   // 移除 const
    void setMat3(const std::string& name, const glm::mat3& mat);     // 移除 const
    void setMat4(const std::string& name, const glm::mat4& mat);     // 移除 const

private:
    void compileShaders(const char* vShaderCode, const char* fShaderCode);
    void checkCompileErrors(unsigned int shader, std::string type);
};

#endif // SHADER_H