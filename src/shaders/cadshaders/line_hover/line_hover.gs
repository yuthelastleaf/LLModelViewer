// shaders/line_thick/line_thick.gs

#version 330 core

// ============================================
// 输入/输出定义
// ============================================

layout(lines) in;                          // 输入：线段（2 个顶点）
layout(triangle_strip, max_vertices = 4) out;  // 输出：三角形条带（4 个顶点）

// ============================================
// Uniforms
// ============================================

uniform vec2 viewport;      // 视口大小（像素）
uniform float thickness;    // 线宽（像素）
uniform mat4 projection;    // 投影矩阵（用于计算深度）

// ============================================
// 输出到片段着色器
// ============================================

out vec2 vUV;  // UV 坐标（可选，用于纹理或渐变）

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
    // NDC [-1,1] → Screen [0, viewport]
    vec2 screen0 = (0.5 * p0.xy/p0.w + 0.5) * viewport;
    vec2 screen1 = (0.5 * p1.xy/p1.w + 0.5) * viewport;
    
    // ────────────────────────────────────────
    // 3. 计算线段方向和垂直方向（屏幕空间）
    // ────────────────────────────────────────
    vec2 dir = screen1 - screen0;
    
    // 处理退化情况（两点重合）
    if (length(dir) < 0.001) {
        return;  // 不输出任何顶点
    }
    
    dir = normalize(dir);              // 归一化方向
    vec2 perp = vec2(-dir.y, dir.x);   // 垂直向量（逆时针 90°）
    
    // ────────────────────────────────────────
    // 4. 计算偏移量（屏幕空间像素）
    // ────────────────────────────────────────
    vec2 offset = perp * thickness * 0.5;  // 半宽
    
    // ────────────────────────────────────────
    // 5. 计算矩形的 4 个角点（屏幕空间）
    // ────────────────────────────────────────
    vec2 screen_p0_top = screen0 + offset;   // A'
    vec2 screen_p0_bot = screen0 - offset;   // A"
    vec2 screen_p1_top = screen1 + offset;   // B'
    vec2 screen_p1_bot = screen1 - offset;   // B"
    
    // ────────────────────────────────────────
    // 6. 转换回 NDC 坐标
    // ────────────────────────────────────────
    // Screen [0, viewport] → NDC [-1,1]
    vec2 ndc_p0_top = (screen_p0_top / viewport) * 2.0 - 1.0;
    vec2 ndc_p0_bot = (screen_p0_bot / viewport) * 2.0 - 1.0;
    vec2 ndc_p1_top = (screen_p1_top / viewport) * 2.0 - 1.0;
    vec2 ndc_p1_bot = (screen_p1_bot / viewport) * 2.0 - 1.0;
    
    // ────────────────────────────────────────
    // 7. 输出 4 个顶点（Triangle Strip 顺序）
    // ────────────────────────────────────────
    
    // 顶点 1：A' (左上)
    gl_Position = vec4(ndc_p0_top * p0.w, p0.z, p0.w);
    vUV = vec2(0.0, 1.0);
    EmitVertex();
    
    // 顶点 2：A" (左下)
    gl_Position = vec4(ndc_p0_bot * p0.w, p0.z, p0.w);
    vUV = vec2(0.0, 0.0);
    EmitVertex();
    
    // 顶点 3：B' (右上)
    gl_Position = vec4(ndc_p1_top * p1.w, p1.z, p1.w);
    vUV = vec2(1.0, 1.0);
    EmitVertex();
    
    // 顶点 4：B" (右下)
    gl_Position = vec4(ndc_p1_bot * p1.w, p1.z, p1.w);
    vUV = vec2(1.0, 0.0);
    EmitVertex();
    
    EndPrimitive();
}