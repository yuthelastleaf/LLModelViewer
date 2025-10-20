#include "GridAxisHelper.h"
#include <cmath>

static glm::vec3 unprojectOnZ0(const glm::mat4& invVP, float nx, float ny) {
    // 从 NDC (nx,ny,-1) 与 (nx,ny,1) 得到一条射线，求与 z=0 的交点
    glm::vec4 ndcNear(nx, ny, -1.0f, 1.0f);
    glm::vec4 ndcFar (nx, ny,  1.0f, 1.0f);
    glm::vec4 p0 = invVP * ndcNear; p0 /= p0.w;
    glm::vec4 p1 = invVP * ndcFar;  p1 /= p1.w;
    glm::vec3 o = glm::vec3(p0);
    glm::vec3 d = glm::normalize(glm::vec3(p1 - p0));
    if (std::abs(d.z) < 1e-6f) return o; // 平行情况退化
    float t = -o.z / d.z;
    return o + t*d;
}

bool GridRenderer::worldRectOnZ0(const ViewportState& vp, glm::vec2& minXY, glm::vec2& maxXY) {
    glm::mat4 invVP = glm::inverse(vp.proj * vp.view);
    // 四角 NDC
    glm::vec3 p00 = unprojectOnZ0(invVP, -1.f, -1.f);
    glm::vec3 p10 = unprojectOnZ0(invVP,  1.f, -1.f);
    glm::vec3 p01 = unprojectOnZ0(invVP, -1.f,  1.f);
    glm::vec3 p11 = unprojectOnZ0(invVP,  1.f,  1.f);

    float minX = std::min(std::min(p00.x, p10.x), std::min(p01.x, p11.x));
    float maxX = std::max(std::max(p00.x, p10.x), std::max(p01.x, p11.x));
    float minY = std::min(std::min(p00.y, p10.y), std::min(p01.y, p11.y));
    float maxY = std::max(std::max(p00.y, p10.y), std::max(p01.y, p11.y));
    
    if (!std::isfinite(minX) || !std::isfinite(maxX) || 
        !std::isfinite(minY) || !std::isfinite(maxY)) return false;
    
    minXY = {minX, minY};
    maxXY = {maxX, maxY};
    return true;
}

// 选择网格间距：让屏幕上相邻网格约 ~ 50~150 px
float GridRenderer::chooseMinorStep(float worldPerPixel) {
    // 目标像素间隔
    float targetPx = 80.0f;
    float step = targetPx * worldPerPixel; // 初步的世界距离
    
    if (step <= 0.0f) return 1.0f;  // 防御性检查
    
    // 归一化为 1*10^k
    float base = std::pow(10.0f, std::floor(std::log10(step)));
    float scaled = step / base; // [1,10)
    float nice;
    if (scaled < 1.5f) nice = 1.0f;
    else if (scaled < 3.5f) nice = 2.0f;
    else if (scaled < 7.5f) nice = 5.0f;
    else nice = 10.0f;
    return nice * base;
}

void GridRenderer::draw(Renderer& r, const ViewportState& vp,
                        std::uint32_t minorColor,
                        std::uint32_t majorColor,
                        int majorEvery)
{
    glm::vec2 minXY, maxXY;
    if (!worldRectOnZ0(vp, minXY, maxXY)) return;

    float minor = chooseMinorStep(vp.worldPerPixel);
    float major = minor * float(majorEvery);

    auto snapDown = [](float v, float step) {
        return std::floor(v / step) * step;
    };

    std::vector<glm::vec3> linesMinor, linesMajor;
    linesMinor.reserve(4096);
    linesMajor.reserve(1024);

    float gridZ = -0.01f;  // 负数表示向后

    // 垂直线
    float xStart = snapDown(minXY.x, minor);
    for (float x = xStart; x <= maxXY.x + 1e-6f; x += minor) {
        // 判断是否为主网格线（接近 major 的倍数）
        bool isMajor = std::fabs(std::fmod(x, major)) < minor * 0.1f;
        auto& dst = isMajor ? linesMajor : linesMinor;
        dst.push_back({x, minXY.y, -0.01f});
        dst.push_back({x, maxXY.y, -0.01f});
    }
    
    // 水平线
    float yStart = snapDown(minXY.y, minor);
    for (float y = yStart; y <= maxXY.y + 1e-6f; y += minor) {
        bool isMajor = std::fabs(std::fmod(y, major)) < minor * 0.1f;
        auto& dst = isMajor ? linesMajor : linesMinor;
        dst.push_back({minXY.x, y, -0.01f});
        dst.push_back({maxXY.x, y, -0.01f});
    }

    if (!linesMinor.empty()) r.drawLineSegments(linesMinor, minorColor, vp);
    if (!linesMajor.empty()) r.drawLineSegments(linesMajor, majorColor, vp);
}

// ========== 坐标轴渲染 ==========
void AxisRenderer::draw(Renderer& r, const ViewportState& vp,
                        float axisLength,
                        std::uint32_t xColor,
                        std::uint32_t yColor,
                        std::uint32_t zColor)
{
    std::vector<glm::vec3> xAxis = {
        {0.0f, 0.0f, 0.0f},
        {axisLength, 0.0f, 0.0f}
    };
    std::vector<glm::vec3> yAxis = {
        {0.0f, 0.0f, 0.0f},
        {0.0f, axisLength, 0.0f}
    };
    std::vector<glm::vec3> zAxis = {
        {0.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, axisLength}
    };
    
    r.drawLineSegments(xAxis, xColor, vp);
    r.drawLineSegments(yAxis, yColor, vp);
    r.drawLineSegments(zAxis, zColor, vp);
}