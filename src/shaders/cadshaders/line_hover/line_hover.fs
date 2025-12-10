// ✨ Enhanced Line Hover Fragment Shader
// 🎯 实现距离场抗锯齿和自然的边缘效果

#version 330 core

// ============================================
// Uniforms
// ============================================

uniform vec4 color;           // 基础颜色
uniform float thickness;      // 线宽（像素）
uniform float roundRadius;    // 圆角半径（像素）
uniform bool enableRounding;  // 是否启用圆角端点
uniform float featherWidth;   // 边缘羽化宽度（像素）
uniform float glowIntensity;  // 发光强度 [0.0-1.0]

// ============================================
// 输入（从几何着色器）
// ============================================

in vec2 vUV;           // UV 坐标
in float vDistance;    // 到线段中心的距离
in vec2 vLocalPos;     // 局部位置

// ============================================
// 输出
// ============================================

out vec4 FragColor;

// ============================================
// 辅助函数 
// ============================================

// 🔹 平滑阶梯函数（抗锯齿）
float smoothstepAA(float edge0, float edge1, float x) {
    return smoothstep(edge0 - 0.5, edge1 + 0.5, x);
}

// 🔹 距离场圆角矩形
float roundedRectSDF(vec2 pos, vec2 size, float radius) {
    vec2 q = abs(pos) - size + radius;
    return length(max(q, 0.0)) + min(max(q.x, q.y), 0.0) - radius;
}

// 🔹 发光效果计算
vec4 addGlow(vec4 baseColor, float distance, float intensity) {
    if (intensity <= 0.0) return baseColor;
    
    // 外发光：距离越远，发光越弱
    float glowFactor = exp(-distance * 0.1) * intensity;
    vec3 glowColor = baseColor.rgb * 1.5; // 稍微提亮
    
    return mix(baseColor, vec4(glowColor, baseColor.a), glowFactor);
}

// ============================================
// 主函数
// ============================================

void main() {
    // ────────────────────────────────────────
    // 1. 计算到线段中心轴的距离
    // ────────────────────────────────────────
    float coreWidth = thickness * 0.5;
    float totalWidth = coreWidth + featherWidth;
    
    // Y方向距离（垂直于线段）
    float distanceToAxis = abs(vLocalPos.y);
    
    // ────────────────────────────────────────
    // 2. 圆角端点处理（可选）
    // ────────────────────────────────────────
    float alpha = 1.0;
    
    if (enableRounding && roundRadius > 0.0) {
        // 计算到圆角矩形的SDF距离
        vec2 rectSize = vec2(length(vLocalPos.x), coreWidth);
        float sdf = roundedRectSDF(vLocalPos, rectSize, roundRadius);
        
        // 基于SDF的抗锯齿alpha
        alpha = 1.0 - smoothstepAA(-featherWidth, 0.0, sdf);
    } else {
        // 标准矩形：基于Y方向距离的抗锯齿
        alpha = 1.0 - smoothstepAA(coreWidth, totalWidth, distanceToAxis);
    }
    
    // ────────────────────────────────────────
    // 3. 边缘羽化效果
    // ────────────────────────────────────────
    // 核心区域：完全不透明
    if (distanceToAxis <= coreWidth) {
        alpha = 1.0;
    } else {
        // 羽化区域：平滑过渡到透明
        float featherFactor = (distanceToAxis - coreWidth) / featherWidth;
        alpha *= 1.0 - smoothstep(0.0, 1.0, featherFactor);
    }
    
    // ────────────────────────────────────────
    // 4. 最终颜色计算
    // ────────────────────────────────────────
    vec4 finalColor = color;
    finalColor.a *= alpha;
    
    // 添加发光效果（可选）
    if (glowIntensity > 0.0) {
        finalColor = addGlow(finalColor, distanceToAxis, glowIntensity);
    }
    
    // 边缘增强：在边界附近稍微提亮
    if (distanceToAxis > coreWidth * 0.8 && alpha > 0.1) {
        float edgeBoost = 1.0 + (1.0 - alpha) * 0.2;
        finalColor.rgb *= edgeBoost;
    }
    
    FragColor = finalColor;
    
    // 🔍 调试：可选的可视化模式
    // FragColor = vec4(vUV, 0.0, 1.0);           // UV可视化
    // FragColor = vec4(vec3(alpha), 1.0);        // Alpha可视化  
    // FragColor = vec4(vec3(distanceToAxis/totalWidth), 1.0); // 距离可视化
}