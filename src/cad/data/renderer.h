#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include <QOpenGLFunctions_3_3_Core>
#include <QPoint>
#include "../../base/util/shader.h"

#include <glm/glm.hpp>
#include "document.h"

struct ViewportState {
    int width = 0, height = 0;
    glm::mat4 view{1.0f}, proj{1.0f};
    float worldPerPixel = 1.0f; // 屏幕 1 像素对应多少世界单位，用于细分
    
    // 计算 worldPerPixel（在视图矩阵更新时调用）
    void updateWorldPerPixel();

    // ============================================
    // 坐标转换方法
    // ============================================
    
    // ✅ 屏幕坐标转世界坐标（在指定 Z 平面上）
    glm::vec3 screenToWorld(int screenX, int screenY, float planeZ = 0.0f) const;
    
    // ✅ 重载：QPoint 版本
    glm::vec3 screenToWorld(const QPoint& screenPos, float planeZ = 0.0f) const;
    
    // ✅ 世界坐标转屏幕坐标
    glm::vec2 worldToScreen(const glm::vec3& worldPos) const;
    
    // ✅ NDC 转世界坐标
    glm::vec3 ndcToWorld(float ndcX, float ndcY, float planeZ = 0.0f) const;
    
    // ✅ 世界坐标转 NDC
    glm::vec3 worldToNDC(const glm::vec3& worldPos) const;
    
    // ============================================
    // 工具方法
    // ============================================
    
    // ✅ 判断世界坐标点是否在视野内
    bool isVisible(const glm::vec3& worldPos) const;
    
    // ✅ 获取世界空间中某点对应的屏幕像素大小
    float getPixelSizeAt(const glm::vec3& worldPos) const;
    
    // ✅ 获取视锥体的 8 个角点（世界坐标）
    void getFrustumCorners(glm::vec3 corners[8]) const;
};

// 最小顶点结构（仅位置）
struct PosVertex { glm::vec3 pos; };

// GPU 批次（v0.1 每实体一个批次）
struct GpuBatch {
    GLuint vao = 0, vbo = 0, ibo = 0;
    GLsizei indexCount = 0;
    std::uint32_t rgba = 0xFFFFFFFF;
    GLenum drawMode = GL_LINES;  // GL_LINES, GL_LINE_STRIP, GL_TRIANGLES
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
    void uploadBox_(EntityId id, const Box& B, std::uint32_t rgba);

    // 折线细分：保证屏幕误差 ~ 0.5 像素
    static std::vector<glm::vec3> tessellateCircle(const Circle& C, float worldEps);
    static std::vector<glm::vec3> tessellateArc(const Arc& A, float worldEps);

    // GL utils
    GLuint makeVao(GLuint vbo, GLuint ibo);
    void freeBatch_(GpuBatch& b);

private:
    
    // ✅ 使用自定义 Shader
    std::unique_ptr<Shader> shaderLines_;

    // 每实体一个批（v0.1 简单实现；后续可合批）
    std::unordered_map<EntityId, GpuBatch> batches_;
    
    // 缓存上次细分时的 worldPerPixel，用于判断是否需要重新细分
    float lastWorldPerPixel_ = -1.0f;
};