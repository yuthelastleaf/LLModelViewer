#version 330 core

uniform vec4 color;

in vec2 vUV;  // 从几何着色器传来的 UV

out vec4 FragColor;

void main() {
    FragColor = color;
    
    // ✅ 可选：添加边缘渐变（反走样）
    // float edgeFade = 1.0 - abs(vUV.y - 0.5) * 2.0;
    // FragColor.a *= smoothstep(0.0, 0.2, edgeFade);
}