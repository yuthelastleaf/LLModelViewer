#pragma once
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <QOpenGLFunctions_3_3_Core>
#include "renderer.h"
#include "../../base/util/shader.h"

class GridRenderer : protected QOpenGLFunctions_3_3_Core {
public:
    GridRenderer();
    ~GridRenderer();
    
    void draw(Renderer& r, const ViewportState& vp,
              std::uint32_t minorColor = 0x40404040,
              std::uint32_t majorColor = 0x80808080,
              int majorEvery = 5);
    
    // 传统线段方式（备用）
    void drawLines(Renderer& r, const ViewportState& vp,
                   std::uint32_t minorColor = 0x40404040,
                   std::uint32_t majorColor = 0x80808080,
                   int majorEvery = 5);

private:
    void initializeGrid();
    void cleanup();
    
    static bool worldRectOnZ0(const ViewportState& vp, glm::vec2& minXY, glm::vec2& maxXY);
    static float chooseMinorStep(float worldPerPixel);
    
    // Shader 方式
    std::unique_ptr<Shader> gridShader_;
    unsigned int gridVAO_, gridVBO_;
    bool initialized_;
};

class AxisRenderer : protected QOpenGLFunctions_3_3_Core {
public:
    AxisRenderer();
    ~AxisRenderer();
    
    void draw(Renderer& r, const ViewportState& vp,
              float axisLength = 1000.0f,
              std::uint32_t xColor = 0xFF0000FF,
              std::uint32_t yColor = 0x00FF00FF,
              std::uint32_t zColor = 0x0000FFFF,
              bool drawZ = true);

private:
    void initializeAxis();
    void cleanup();
    
    std::unique_ptr<Shader> axisShader_;
    unsigned int axisVAO_, axisVBO_;
    bool initialized_;
};