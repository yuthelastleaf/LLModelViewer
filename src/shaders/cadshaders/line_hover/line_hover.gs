// ✨ Enhanced Line Hover Geometry Shader
// 🎯 解决矩形拼凑感，实现自然的线条高亮效果

#version 330 core

// ============================================
// 输入/输出定义
// ============================================

layout(lines) in;                          // 输入：线段（2 个顶点）
layout(triangle_strip, max_vertices = 10) out; // 输出：增加顶点数支持圆角

// ============================================
// Uniforms - 增强配置
// ============================================

uniform vec2 viewport;        // 视口大小（像素）
uniform float thickness;      // 线宽（像素）
uniform float roundRadius;    // 圆角半径（像素）[新增]
uniform bool enableRounding;  // 是否启用圆角端点 [新增]
uniform float featherWidth;   // 边缘羽化宽度（像素）[新增]

// ============================================
// 输出到片段着色器
// ============================================

out vec2 vUV;           // UV 坐标
out float vDistance;    // 到线段中心的距离 [新增：用于抗锯齿]
out vec2 vLocalPos;     // 局部位置 [新增：用于圆角计算]

// ============================================
// 辅助函数
// ============================================

// 🔹 将NDC坐标转换为屏幕像素坐标
vec2 ndcToScreen(vec4 ndc) {
    return (0.5 * ndc.xy/ndc.w + 0.5) * viewport;
}

// 🔹 将屏幕像素坐标转换为NDC坐标
vec2 screenToNdc(vec2 screen, float w) {
    return ((screen / viewport) * 2.0 - 1.0) * w;
}

// 🔹 输出一个顶点（封装重复代码）
void emitVertex(vec2 screenPos, vec4 originalPos, vec2 uv, float distance, vec2 localPos) {
    vec2 ndc = screenToNdc(screenPos, originalPos.w);
    gl_Position = vec4(ndc, originalPos.z, originalPos.w);
    vUV = uv;
    vDistance = distance;
    vLocalPos = localPos;
    EmitVertex();
}

// ============================================
// 主函数
// ============================================

void main() {
    // ────────────────────────────────────────
    // 1. 获取输入的两个顶点（NDC 坐标）
    // ────────────────────────────────────────
    vec4 p0 = gl_in[0].gl_Position;  // 起点 A
    vec4 p1 = gl_in[1].gl_Position;  // 终点 B
    
    // ────────────────────────────────────────
    // 2. 转换到屏幕空间（像素坐标）
    // ────────────────────────────────────────
    vec2 screen0 = ndcToScreen(p0);
    vec2 screen1 = ndcToScreen(p1);
    
    // ────────────────────────────────────────
    // 3. 计算线段几何属性
    // ────────────────────────────────────────
    vec2 dir = screen1 - screen0;
    float lineLength = length(dir);
    
    // 处理退化情况（两点重合或过短）
    if (lineLength < 0.1) {
        return;  // 不输出任何顶点
    }
    
    dir = dir / lineLength;              // 归一化方向
    vec2 perp = vec2(-dir.y, dir.x);     // 垂直向量（逆时针 90°）
    
    // ────────────────────────────────────────
    // 4. 计算增强的线宽（包含羽化区域）
    // ────────────────────────────────────────
    float totalWidth = thickness + featherWidth * 2.0;
    float halfWidth = totalWidth * 0.5;
    
    // ────────────────────────────────────────
    // 5. 线段延伸（支持圆角端点）
    // ────────────────────────────────────────
    float extension = enableRounding ? roundRadius : 0.0;
    vec2 extendedStart = screen0 - dir * extension;
    vec2 extendedEnd = screen1 + dir * extension;
    
    // ────────────────────────────────────────
    // 6. 计算矩形的 4 个角点（屏幕空间）
    // ────────────────────────────────────────
    vec2 offset = perp * halfWidth;
    
    vec2 p0_top = extendedStart + offset;   // A'
    vec2 p0_bot = extendedStart - offset;   // A"
    vec2 p1_top = extendedEnd + offset;     // B'
    vec2 p1_bot = extendedEnd - offset;     // B"
    
    // ────────────────────────────────────────
    // 7. 输出增强的矩形（Triangle Strip）
    // ────────────────────────────────────────
    // 🎯 关键改进：提供更多信息给片段着色器做抗锯齿
    
    // 顶点 1：A' (左上)
    emitVertex(p0_top, p0, vec2(0.0, 1.0), halfWidth, vec2(-extension, halfWidth));
    
    // 顶点 2：A" (左下)  
    emitVertex(p0_bot, p0, vec2(0.0, 0.0), halfWidth, vec2(-extension, -halfWidth));
    
    // 顶点 3：B' (右上)
    emitVertex(p1_top, p1, vec2(1.0, 1.0), halfWidth, vec2(lineLength + extension, halfWidth));
    
    // 顶点 4：B" (右下)
    emitVertex(p1_bot, p1, vec2(1.0, 0.0), halfWidth, vec2(lineLength + extension, -halfWidth));
    
    EndPrimitive();
}