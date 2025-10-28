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
    
    // ============================================
    // 构造函数
    // ============================================
    
    // 从文件路径构造（顶点 + 片段）
    Shader(const char* vertexPath, const char* fragmentPath);
    
    // ✅ 从文件路径构造（顶点 + 几何 + 片段）
    Shader(const char* vertexPath, 
           const char* geometryPath, 
           const char* fragmentPath);
    
    // 从字符串内容构造
    Shader(const char* vertexContent, const char* fragmentContent, bool fromString);
    
    // ✅ 从字符串内容构造（带几何着色器）
    Shader(const char* vertexContent, 
           const char* geometryContent,
           const char* fragmentContent, 
           bool fromString);
    
    ~Shader();
    
    // ============================================
    // 使用着色器
    // ============================================
    
    void use();
    
    // ============================================
    // Uniform 工具函数
    // ============================================
    
    void setBool(const std::string& name, bool value);
    void setInt(const std::string& name, int value);
    void setFloat(const std::string& name, float value);
    void setVec2(const std::string& name, const glm::vec2& value);
    void setVec3(const std::string& name, float x, float y, float z);
    void setVec3(const std::string& name, const glm::vec3& value);
    void setVec4(const std::string& name, const glm::vec4& value);
    void setMat3(const std::string& name, const glm::mat3& mat);
    void setMat4(const std::string& name, const glm::mat4& mat);

private:
    // ============================================
    // 编译和链接
    // ============================================
    
    // 编译着色器（顶点 + 片段）
    void compileShaders(const char* vShaderCode, const char* fShaderCode);
    
    // ✅ 编译着色器（顶点 + 几何 + 片段）
    void compileShaders(const char* vShaderCode, 
                       const char* gShaderCode,
                       const char* fShaderCode);
    
    // 检查编译错误
    void checkCompileErrors(unsigned int shader, std::string type);
};

#endif // SHADER_H