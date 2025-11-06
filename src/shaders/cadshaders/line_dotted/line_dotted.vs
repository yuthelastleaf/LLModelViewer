#version 330 core

// 顶点位置
layout (location = 0) in vec3 aPos;
// 沿线段的累积距离（在VBO中第二个属性）
layout (location = 1) in float aDist;

uniform mat4 mvp;

// 传递沿线段的累积距离到片段着色器
out float distanceAlongLine;

void main() {
    gl_Position = mvp * vec4(aPos, 1.0);
    // 直接使用aDist属性作为距离信息
    distanceAlongLine = aDist;
}
