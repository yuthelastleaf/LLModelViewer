#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "renderer.h" // ViewportState + Renderer

class GridRenderer {
public:
    // 主网格每隔 majorStep = minorStep * majorEvery
    void draw(Renderer& r, const ViewportState& vp,
              std::uint32_t minorColor = 0x30FFFFFF30,  // RGBA: 半透明白
              std::uint32_t majorColor = 0x60FFFFFF60,  // RGBA: 更亮的白
              int majorEvery = 10);

private:
    static bool worldRectOnZ0(const ViewportState& vp, glm::vec2& minXY, glm::vec2& maxXY);
    static float chooseMinorStep(float worldPerPixel);
};

class AxisRenderer {
public:
    // 绘制 XYZ 坐标轴（原点到指定长度）
    void draw(Renderer& r, const ViewportState& vp,
              float axisLength = 1000.0f,
              std::uint32_t xColor = 0xFF0000FF,  // 红色 X
              std::uint32_t yColor = 0x00FF00FF,  // 绿色 Y
              std::uint32_t zColor = 0x0000FFFF); // 蓝色 Z
};