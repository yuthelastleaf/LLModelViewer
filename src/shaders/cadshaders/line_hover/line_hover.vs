#version 330 core

layout (location = 0) in vec3 aPos;

uniform mat4 mvp;

void main() {
    // 只做 MVP 变换，不输出到屏幕
    // 几何着色器会进一步处理
    gl_Position = mvp * vec4(aPos, 1.0);
}