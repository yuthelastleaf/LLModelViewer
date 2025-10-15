#version 330 core

// 从顶点着色器接收的输入
in vec3 vertexColor;

// 输出颜色
out vec4 FragColor;

void main()
{
    // 直接使用插值后的顶点颜色
    FragColor = vec4(vertexColor, 1.0);
}