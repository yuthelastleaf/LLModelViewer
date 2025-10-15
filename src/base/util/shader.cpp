#include "Shader.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <fstream>
#include <sstream>
#include <iostream>

Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    // 初始化 OpenGL 函数
    initializeOpenGLFunctions();
    
    std::string vertexCode;
    std::string fragmentCode;
    std::ifstream vShaderFile;
    std::ifstream fShaderFile;

    vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
    
    try
    {
        vShaderFile.open(vertexPath);
        fShaderFile.open(fragmentPath);
        std::stringstream vShaderStream, fShaderStream;
        vShaderStream << vShaderFile.rdbuf();
        fShaderStream << fShaderFile.rdbuf();
        vShaderFile.close();
        fShaderFile.close();
        vertexCode = vShaderStream.str();
        fragmentCode = fShaderStream.str();
        
        qDebug() << "Shader files loaded successfully:";
        qDebug() << "  Vertex:" << vertexPath;
        qDebug() << "  Fragment:" << fragmentPath;
    }
    catch (std::ifstream::failure& e)
    {
        qCritical() << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ:" << e.what();
        qCritical() << "  Vertex path:" << vertexPath;
        qCritical() << "  Fragment path:" << fragmentPath;
        ID = 0;
        return;
    }
    
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    compileShaders(vShaderCode, fShaderCode);
}

Shader::Shader(const char* vertexContent, const char* fragmentContent, bool fromString)
{
    // 初始化 OpenGL 函数
    initializeOpenGLFunctions();
    
    if (fromString) {
        qDebug() << "Creating shader from string content";
        compileShaders(vertexContent, fragmentContent);
    }
}

Shader::~Shader()
{
    if (ID != 0) {
        glDeleteProgram(ID);
        qDebug() << "Shader program deleted, ID:" << ID;
    }
}

void Shader::use()
{
    glUseProgram(ID);
}

// 移除所有函数的 const 修饰符
void Shader::setBool(const std::string& name, bool value)
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value);
}

void Shader::setInt(const std::string& name, int value)
{
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setFloat(const std::string& name, float value)
{
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value);
}

void Shader::setVec3(const std::string& name, float x, float y, float z)
{
    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
}

void Shader::setVec2(const std::string& name, const glm::vec2& value)
{
    glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Shader::setVec3(const std::string& name, const glm::vec3& value)
{
    glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Shader::setVec4(const std::string& name, const glm::vec4& value)
{
    glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]);
}

void Shader::setMat4(const std::string& name, const glm::mat4& mat)
{
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::setMat3(const std::string& name, const glm::mat3& mat)
{
    glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, glm::value_ptr(mat));
}

void Shader::compileShaders(const char* vShaderCode, const char* fShaderCode)
{
    unsigned int vertex, fragment;

    // 编译顶点着色器
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    checkCompileErrors(vertex, "VERTEX");

    // 编译片段着色器
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    checkCompileErrors(fragment, "FRAGMENT");

    // 链接着色器程序
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    checkCompileErrors(ID, "PROGRAM");

    // 删除着色器
    glDeleteShader(vertex);
    glDeleteShader(fragment);
    
    qDebug() << "Shader program created successfully, ID:" << ID;
}

void Shader::checkCompileErrors(unsigned int shader, std::string type)
{
    int success;
    char infoLog[1024];
    
    if (type != "PROGRAM")
    {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success)
        {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            qCritical() << "ERROR::SHADER_COMPILATION_ERROR of type:" << type.c_str();
            qCritical() << infoLog;
            qCritical() << "-- --------------------------------------------------- --";
        }
        else
        {
            qDebug() << type.c_str() << "shader compiled successfully";
        }
    }
    else
    {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success)
        {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            qCritical() << "ERROR::PROGRAM_LINKING_ERROR of type:" << type.c_str();
            qCritical() << infoLog;
            qCritical() << "-- --------------------------------------------------- --";
        }
        else
        {
            qDebug() << "Shader program linked successfully";
        }
    }
}