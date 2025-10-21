#version 330 core

in vec3 worldPos;
out vec4 FragColor;

uniform float gridMinor;     // 次要网格间距
uniform float gridMajor;     // 主要网格间距
uniform vec4 minorColor;     // 次要网格颜色
uniform vec4 majorColor;     // 主要网格颜色
uniform float worldPerPixel; // 缩放级别
uniform float fadeNear;      // 开始淡出的距离（像素）
uniform float fadeFar;       // 完全消失的距离（像素）

void main()
{
    vec2 coord = worldPos.xy;
    
    // 计算到最近网格线的距离（使用导数实现抗锯齿）
    vec2 derivative = fwidth(coord);
    
    // 次要网格线
    vec2 gridMinor2 = abs(fract(coord / gridMinor - 0.5) - 0.5) / derivative * gridMinor;
    float lineMinor = min(gridMinor2.x, gridMinor2.y);
    
    // 主要网格线
    vec2 gridMajor2 = abs(fract(coord / gridMajor - 0.5) - 0.5) / derivative * gridMajor;
    float lineMajor = min(gridMajor2.x, gridMajor2.y);
    
    // 抗锯齿（平滑过渡）
    float minorAlpha = 1.0 - smoothstep(0.0, 1.5, lineMinor);
    float majorAlpha = 1.0 - smoothstep(0.0, 1.5, lineMajor);
    
    // 混合次要和主要网格线
    vec4 color = mix(minorColor, majorColor, majorAlpha);
    color.a *= max(minorAlpha, majorAlpha);
    
    // 根据缩放级别淡出次要网格
    float pixelSpacing = gridMinor / worldPerPixel;
    if (pixelSpacing < fadeFar) {
        float fade = smoothstep(fadeNear, fadeFar, pixelSpacing);
        if (minorAlpha > majorAlpha) {
            color.a *= fade;
        }
    }
    
    // 丢弃完全透明的片段
    if (color.a < 0.01) {
        discard;
    }
    
    FragColor = color;
}