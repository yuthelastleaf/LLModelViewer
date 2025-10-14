#version 330 core

// 输入顶点属性
layout (location = 0) in vec3 aPos;    // 位置
layout (location = 1) in vec3 aColor;  // 颜色

// 输出到片段着色器
out vec3 vertexColor;

// Uniform 变量
uniform mat4 mvp;  // Model-View-Projection 矩阵

void main()
{
    // 应用 MVP 变换
    gl_Position = mvp * vec4(aPos, 1.0);
    
    // 传递颜色到片段着色器
    vertexColor = aColor;
}