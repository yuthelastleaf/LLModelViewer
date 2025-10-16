#pragma once
#include <unordered_map>
#include <vector>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <glm/glm.hpp>
#include "document.h"

struct ViewportState {
    int width = 0, height = 0;
    glm::mat4 view{1.0f}, proj{1.0f};
    float worldPerPixel = 1.0f; // 屏幕 1 像素对应多少世界单位，用于细分
    
    // 计算 worldPerPixel（在视图矩阵更新时调用）
    void updateWorldPerPixel();
};

// 最小顶点结构（仅位置）
struct PosVertex { glm::vec3 pos; };

// GPU 批次（v0.1 每实体一个批次）
struct GpuBatch {
    GLuint vao = 0, vbo = 0, ibo = 0;
    GLsizei indexCount = 0;
    std::uint32_t rgba = 0xFFFFFFFF;
    GLenum drawMode = GL_LINES;  // GL_LINES 或 GL_LINE_STRIP
    // v0.1: lineWidth 先忽略（保持 1px）
};

class Renderer : protected QOpenGLFunctions_3_3_Core {
public:
    Renderer() = default;

    bool initialize(); // 编译最小线条 shader
    void shutdown();

    // 增量同步：仅上传脏实体
    void syncFromDocument(const Document& doc, const ViewportState& vp, bool forceRebuild = false);
    
    // 移除单个实体的批次
    void removeBatch(EntityId id);

    // 绘制所有批次
    void draw(const ViewportState& vp);

    // 低阶画线（供网格/坐标轴等临时使用）
    void drawLineStrip(const std::vector<glm::vec3>& pts, std::uint32_t rgba, const ViewportState& vp);
    void drawLineSegments(const std::vector<glm::vec3>& ptsPairs, std::uint32_t rgba, const ViewportState& vp);

private:
    // 上传 helpers
    void uploadLine_(EntityId id, const Line& L, std::uint32_t rgba);
    void uploadPolyline_(EntityId id, const Polyline& P, std::uint32_t rgba);
    void uploadCircle_(EntityId id, const Circle& C, std::uint32_t rgba, const ViewportState& vp);
    void uploadArc_(EntityId id, const Arc& A, std::uint32_t rgba, const ViewportState& vp);

    // 折线细分：保证屏幕误差 ~ 0.5 像素
    static std::vector<glm::vec3> tessellateCircle(const Circle& C, float worldEps);
    static std::vector<glm::vec3> tessellateArc(const Arc& A, float worldEps);

    // GL utils
    GLuint makeVao(GLuint vbo, GLuint ibo);
    void freeBatch_(GpuBatch& b);

private:
    QOpenGLShaderProgram progLines_;
    GLint uMVP_ = -1, uColor_ = -1;

    // 每实体一个批（v0.1 简单实现；后续可合批）
    std::unordered_map<EntityId, GpuBatch> batches_;
    
    // 缓存上次细分时的 worldPerPixel，用于判断是否需要重新细分
    float lastWorldPerPixel_ = -1.0f;
};