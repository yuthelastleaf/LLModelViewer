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

// 扩展顶点结构（位置 + 沿线距离，用于虚线着色器）
struct PosDistVertex { 
    glm::vec3 pos;      // 3D位置（保持z=0用于2D）
    float dist = 0.0f;  // 沿线段的累积距离
};

// GPU 批次（v0.1 每实体一个批次）
struct GpuBatch {
    GLuint vao = 0, vbo = 0, ibo = 0;
    GLsizei indexCount = 0;
    std::uint32_t rgba = 0xFFFFFFFF;
    GLenum drawMode = GL_LINES;  // GL_LINES, GL_LINE_STRIP, GL_TRIANGLES
    // ✅ v0.2: 选择状态
    bool selected = false;    // 是否被选中
    bool hovered = false;   // ✅ 悬停状态
    bool doted = false;     // 是否虚线状态
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

    // ✅ v0.2: 高亮颜色设置
    void setSelectionColor(std::uint32_t rgba) { selectionColor_ = rgba; }
    std::uint32_t getSelectionColor() const { return selectionColor_; }
    
    void setSelectionWidth(float width) { selectionWidth_ = width; }
    float getSelectionWidth() const { return selectionWidth_; }

    // ✅ 悬停高亮颜色
    void setHoverColor(std::uint32_t rgba) { hoverColor_ = rgba; }
    std::uint32_t getHoverColor() const { return hoverColor_; }

public:
    struct HoverStyle {
        std::uint32_t color;        // 主颜色
        float lineWidth;            // 线宽（像素）
        bool enableGlow;            // 是否启用发光效果
        std::uint32_t glowColor;    // 发光颜色
        float glowWidth;            // 发光宽度（像素）
        
        // ✅ 默认构造函数（提供默认值）
        HoverStyle()
            : color(0x00FFFFFF)         // 青色
            , lineWidth(2.0f)
            , enableGlow(true)
            , glowColor(0x00FFFF80)     // 半透明青色
            , glowWidth(4.0f)
        {}
    };
    
    struct SelectionStyle {
        std::uint32_t color;
        float lineWidth;
        bool enableGlow;
        std::uint32_t glowColor;
        float glowWidth;
        
        // ✅ 默认构造函数（提供默认值）
        SelectionStyle()
            : color(0xFF9900FF)         // 橙色
            , lineWidth(2.0f)
            , enableGlow(true)
            , glowColor(0xFF990080)     // 半透明橙色
            , glowWidth(4.0f)
        {}
    };

private:
    // 上传 helpers
    void uploadLine_(EntityId id, const Line& L, std::uint32_t rgba);
    void uploadRectangle_(EntityId id, const Rectangle& L, std::uint32_t rgba);
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
    std::unique_ptr<Shader> shader_hover_Lines_;
    std::unique_ptr<Shader> shader_hover_Solid_;
    std::unique_ptr<Shader> shaderDottedLines_;  // 虚线着色器

    // 每实体一个批（v0.1 简单实现；后续可合批）
    std::unordered_map<EntityId, GpuBatch> batches_;
    
    // 缓存上次细分时的 worldPerPixel，用于判断是否需要重新细分
    float lastWorldPerPixel_ = -1.0f;

    // ✅ v0.2: 高亮渲染参数
    std::uint32_t selectionColor_ = 0xFFFFFFFF;  // 橙色
    std::uint32_t hoverColor_ = 0x00FFFFFF;      // 悬停：青色
    float selectionWidth_ = 2.0f;                 // 线宽（未来支持）

    HoverStyle hoverStyle_;
    SelectionStyle selectionStyle_;
};