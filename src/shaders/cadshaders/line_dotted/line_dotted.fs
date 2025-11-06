#version 330 core

in float distanceAlongLine;

uniform vec4 color;
uniform float dashLength;     // 虚线段长度（实线部分）
uniform float gapLength;      // 间隙长度（空白部分）

out vec4 FragColor;

void main() {
    float period = dashLength + gapLength;
    float pos = mod(distanceAlongLine, period);
    
    // 如果在虚线段内，绘制；否则丢弃
    if (pos < dashLength) {
        FragColor = color;
    } else {
        discard;
    }
}
