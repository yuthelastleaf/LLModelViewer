#include "GridAxisHelper.h"
#include <cmath>
#include <QDebug>

// ============================================
// 辅助函数
// ============================================

static glm::vec3 unprojectOnZ0(const glm::mat4 &invVP, float nx, float ny)
{
    glm::vec4 ndcNear(nx, ny, -1.0f, 1.0f);
    glm::vec4 ndcFar(nx, ny, 1.0f, 1.0f);
    glm::vec4 p0 = invVP * ndcNear;
    p0 /= p0.w;
    glm::vec4 p1 = invVP * ndcFar;
    p1 /= p1.w;
    glm::vec3 o = glm::vec3(p0);
    glm::vec3 d = glm::normalize(glm::vec3(p1 - p0));
    if (std::abs(d.z) < 1e-6f)
        return o;
    float t = -o.z / d.z;
    return o + t * d;
}

bool GridRenderer::worldRectOnZ0(const ViewportState &vp, glm::vec2 &minXY, glm::vec2 &maxXY)
{
    glm::mat4 invVP = glm::inverse(vp.proj * vp.view);
    glm::vec3 p00 = unprojectOnZ0(invVP, -1.f, -1.f);
    glm::vec3 p10 = unprojectOnZ0(invVP, 1.f, -1.f);
    glm::vec3 p01 = unprojectOnZ0(invVP, -1.f, 1.f);
    glm::vec3 p11 = unprojectOnZ0(invVP, 1.f, 1.f);

    float minX = std::min(std::min(p00.x, p10.x), std::min(p01.x, p11.x));
    float maxX = std::max(std::max(p00.x, p10.x), std::max(p01.x, p11.x));
    float minY = std::min(std::min(p00.y, p10.y), std::min(p01.y, p11.y));
    float maxY = std::max(std::max(p00.y, p10.y), std::max(p01.y, p11.y));

    if (!std::isfinite(minX) || !std::isfinite(maxX) ||
        !std::isfinite(minY) || !std::isfinite(maxY))
        return false;

    minXY = {minX, minY};
    maxXY = {maxX, maxY};
    return true;
}

float GridRenderer::chooseMinorStep(float worldPerPixel)
{
    float targetPx = 80.0f;
    float step = targetPx * worldPerPixel;

    if (step <= 0.0f)
        return 1.0f;

    float base = std::pow(10.0f, std::floor(std::log10(step)));
    float scaled = step / base;
    float nice;
    if (scaled < 1.5f)
        nice = 1.0f;
    else if (scaled < 3.5f)
        nice = 2.0f;
    else if (scaled < 7.5f)
        nice = 5.0f;
    else
        nice = 10.0f;
    return nice * base;
}

// ============================================
// GridRenderer - Shader 实现
// ============================================

GridRenderer::GridRenderer()
    : gridVAO_(0), gridVBO_(0), initialized_(false)
{
}

GridRenderer::~GridRenderer()
{
    cleanup();
}

void GridRenderer::initializeGrid()
{
    if (initialized_)
        return;

    initializeOpenGLFunctions();

    try
    {
        gridShader_ = std::make_unique<Shader>(
            "shaders/grid/grid.vs",
            "shaders/grid/grid.fs");
    }
    catch (const std::exception &e)
    {
        qCritical() << "Failed to create grid shader:" << e.what();
        return;
    }

    // ✅ 创建一个覆盖整个视口的大四边形
    // 在 draw() 中会根据实际视口范围动态调整
    float vertices[] = {
        -1.0f, -1.0f, -0.02f, // 左下
        1.0f, -1.0f, -0.02f,  // 右下
        1.0f, 1.0f, -0.02f,   // 右上
        -1.0f, 1.0f, -0.02f   // 左上
    };

    glGenVertexArrays(1, &gridVAO_);
    glGenBuffers(1, &gridVBO_);

    glBindVertexArray(gridVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    initialized_ = true;
    qDebug() << "GridRenderer initialized with shader";
}

void GridRenderer::cleanup()
{
    if (gridVAO_)
        glDeleteVertexArrays(1, &gridVAO_);
    if (gridVBO_)
        glDeleteBuffers(1, &gridVBO_);
    gridVAO_ = gridVBO_ = 0;
    initialized_ = false;
}

void GridRenderer::draw(Renderer &r, const ViewportState &vp,
                        std::uint32_t minorColor,
                        std::uint32_t majorColor,
                        int majorEvery)
{
    if (!initialized_)
    {
        initializeGrid();
    }

    glm::vec2 minXY, maxXY;
    if (!worldRectOnZ0(vp, minXY, maxXY))
        return;

    // 计算网格间距
    float minor = chooseMinorStep(vp.worldPerPixel);
    float major = minor * float(majorEvery);

    // 转换颜色为 vec4
    auto colorToVec4 = [](std::uint32_t color)
    {
        float r = ((color >> 24) & 0xFF) / 255.0f;
        float g = ((color >> 16) & 0xFF) / 255.0f;
        float b = ((color >> 8) & 0xFF) / 255.0f;
        float a = (color & 0xFF) / 255.0f;
        return glm::vec4(r, g, b, a);
    };

    glm::vec4 minorCol = colorToVec4(minorColor);
    glm::vec4 majorCol = colorToVec4(majorColor);

    // 更新四边形顶点以覆盖可见区域
    float padding = (maxXY.x - minXY.x) * 0.1f; // 10% 边距
    float vertices[] = {
        minXY.x - padding, minXY.y - padding, -0.0f,
        maxXY.x + padding, minXY.y - padding, -0.0f,
        maxXY.x + padding, maxXY.y + padding, -0.0f,
        minXY.x - padding, maxXY.y + padding, -0.0f};

    glBindBuffer(GL_ARRAY_BUFFER, gridVBO_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

     // ✅ 使用深度测试 + 禁用深度写入，让网格在背景但不影响深度缓冲
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);  // 允许等深度通过
    glDepthMask(GL_FALSE);   // 禁止写入深度缓冲
    // 启用混合
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 使用 shader
    gridShader_->use();
    gridShader_->setMat4("model", glm::mat4(1.0f));
    gridShader_->setMat4("view", vp.view);
    gridShader_->setMat4("projection", vp.proj);
    gridShader_->setFloat("gridMinor", minor);
    gridShader_->setFloat("gridMajor", major);
    gridShader_->setVec4("minorColor", minorCol);
    gridShader_->setVec4("majorColor", majorCol);
    gridShader_->setFloat("worldPerPixel", vp.worldPerPixel);
    gridShader_->setFloat("fadeNear", 30.0f);
    gridShader_->setFloat("fadeFar", 60.0f);

    // 绘制四边形
    glBindVertexArray(gridVAO_);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    glBindVertexArray(0);
}

// ✅ 保留传统线段方式作为备用
void GridRenderer::drawLines(Renderer &r, const ViewportState &vp,
                             std::uint32_t minorColor,
                             std::uint32_t majorColor,
                             int majorEvery)
{
    // ... 原来的线段绘制代码 ...
}

// ============================================
// AxisRenderer
// ============================================

AxisRenderer::AxisRenderer()
    : axisVAO_(0), axisVBO_(0), initialized_(false)
{
}

AxisRenderer::~AxisRenderer()
{
    cleanup();
}

void AxisRenderer::initializeAxis()
{
    if (initialized_)
        return;

    initializeOpenGLFunctions();

    try
    {
        axisShader_ = std::make_unique<Shader>(
            "shaders/axis/axis.vs",
            "shaders/axis/axis.fs");
    }
    catch (const std::exception &e)
    {
        qCritical() << "Failed to create grid shader:" << e.what();
        return;
    }

    glGenVertexArrays(1, &axisVAO_);
    glGenBuffers(1, &axisVBO_);

    initialized_ = true;
    qDebug() << "AxisRenderer initialized with shader";
}

void AxisRenderer::cleanup()
{
    if (axisVAO_)
        glDeleteVertexArrays(1, &axisVAO_);
    if (axisVBO_)
        glDeleteBuffers(1, &axisVBO_);
    axisVAO_ = axisVBO_ = 0;
    initialized_ = false;
}

void AxisRenderer::draw(Renderer& r, const ViewportState& vp,
                        float axisLength,
                        std::uint32_t xColor,
                        std::uint32_t yColor,
                        std::uint32_t zColor,
                        bool drawZ)
{
    if (!initialized_) {
        initializeAxis();
    }

    auto colorToVec4 = [](std::uint32_t color) {
        float r = ((color >> 24) & 0xFF) / 255.0f;
        float g = ((color >> 16) & 0xFF) / 255.0f;
        float b = ((color >> 8) & 0xFF) / 255.0f;
        float a = (color & 0xFF) / 255.0f;
        return glm::vec4(r, g, b, a);
    };

    glm::vec4 xCol = colorToVec4(xColor);
    glm::vec4 yCol = colorToVec4(yColor);
    glm::vec4 zCol = colorToVec4(zColor);

    // ✅ 总是准备完整的 3 轴数据（简单！）
    float vertices[] = {
        // X 轴（顶点 0, 1）
        0.0f, 0.0f, 0.0f,  xCol.r, xCol.g, xCol.b, 1.0f,
        axisLength, 0.0f, 0.0f,  xCol.r, xCol.g, xCol.b, 1.0f,
        // Y 轴（顶点 2, 3）
        0.0f, 0.0f, 0.0f,  yCol.r, yCol.g, yCol.b, yCol.a,
        0.0f, axisLength, 0.0f,  yCol.r, yCol.g, yCol.b, yCol.a,
        // Z 轴（顶点 4, 5）
        0.0f, 0.0f, 0.0f,  zCol.r, zCol.g, zCol.b, zCol.a,
        0.0f, 0.0f, axisLength,  zCol.r, zCol.g, zCol.b, zCol.a
    };

    glBindVertexArray(axisVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, axisVBO_);
    
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glLineWidth(2.0f);

    axisShader_->use();
    axisShader_->setMat4("model", glm::mat4(1.0f));
    axisShader_->setMat4("view", vp.view);
    axisShader_->setMat4("projection", vp.proj);

    
    // ✅ 如果需要，再绘制 Z 轴（顶点 4-5，共 2 个顶点 = 1 条线）
    if (drawZ) {
        glDrawArrays(GL_LINES, 0, 6);
    } else {
        // ✅ 先绘制 X 和 Y 轴（顶点 0-3，共 4 个顶点 = 2 条线）
        glDrawArrays(GL_LINES, 0, 4);
    }

    glDepthFunc(GL_LESS);
    glDisable(GL_LINE_SMOOTH);
    glLineWidth(1.0f);
    glBindVertexArray(0);
}