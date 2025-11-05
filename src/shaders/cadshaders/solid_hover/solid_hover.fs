#version 330 core

uniform vec4 color;

out vec4 FragColor;

void main() {
    // 直接输出颜色，不做任何处理
    FragColor = color;
}