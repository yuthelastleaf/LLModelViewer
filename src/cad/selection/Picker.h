#pragma once
#include "../data/document.h"
#include "../data/renderer.h"
#include "../../base/util/RayUtils.h"
#include <vector>
#include <optional>

/**
 * Picker - 实体拾取器
 * 
 * 功能：
 * - 射线拾取：通过射线与实体求交
 * - 框选拾取：矩形框内的实体
 * - 距离排序：返回最近的实体
 */
class Picker {
public:
    /**
     * 拾取结果
     */
    struct PickResult {
        EntityId entityId;       // 实体 ID
        glm::vec3 hitPoint;      // 交点位置
        float distance;          // 距离（从射线原点）
        
        bool operator<(const PickResult& other) const {
            return distance < other.distance;
        }
    };

public:
    Picker() = default;

    // ============================================
    // 单点拾取（射线）
    // ============================================
    
    /**
     * 从屏幕坐标拾取实体
     * @param screenX 屏幕 X 坐标
     * @param screenY 屏幕 Y 坐标
     * @param document 文档
     * @param vp 视口状态
     * @param threshold 拾取阈值（世界单位）
     * @return 拾取结果（距离最近的）
     */
    std::optional<PickResult> pick(
        int screenX, int screenY,
        const Document& document,
        const ViewportState& vp,
        float threshold = 0.1f) const;
    
    /**
     * 使用射线拾取实体
     * @param ray 射线
     * @param document 文档
     * @param threshold 拾取阈值
     * @return 拾取结果（距离最近的）
     */
    std::optional<PickResult> pick(
        const Ray& ray,
        const Document& document,
        float threshold = 0.1f) const;
    
    /**
     * 拾取所有相交的实体
     * @param ray 射线
     * @param document 文档
     * @param threshold 拾取阈值
     * @return 所有拾取结果（按距离排序）
     */
    std::vector<PickResult> pickAll(
        const Ray& ray,
        const Document& document,
        float threshold = 0.1f) const;

    // ============================================
    // 框选拾取（矩形）
    // ============================================
    
    /**
     * 框选拾取（屏幕空间矩形框）
     * @param minX 矩形左边界（屏幕坐标）
     * @param minY 矩形上边界
     * @param maxX 矩形右边界
     * @param maxY 矩形下边界
     * @param document 文档
     * @param vp 视口状态
     * @param mode 选择模式（INTERSECT=相交, CONTAIN=完全包含）
     * @return 选中的实体 ID 列表
     */
    enum class BoxSelectMode {
        INTERSECT,  // 只要相交就选中
        CONTAIN     // 必须完全包含
    };
    
    std::vector<EntityId> pickBox(
        int minX, int minY,
        int maxX, int maxY,
        const Document& document,
        const ViewportState& vp,
        BoxSelectMode mode = BoxSelectMode::INTERSECT) const;

    // ============================================
    // ✅ v0.2: 2D 拾取（正交视图）
    // ============================================
    
    /**
     * 2D 模式下的拾取（直接使用屏幕/世界坐标）
     * @param worldPos 世界坐标点击位置
     * @param document 文档
     * @param vp 视口状态
     * @param pixelThreshold 拾取阈值（像素单位）
     * @return 拾取结果
     */
    std::optional<PickResult> pick2D(
        const glm::vec3& worldPos,
        const Document& document,
        const ViewportState& vp,
        float pixelThreshold = 5.0f) const;
    
    /**
     * 2D 模式下拾取所有实体
     */
    std::vector<PickResult> pickAll2D(
        const glm::vec3& worldPos,
        const Document& document,
        const ViewportState& vp,
        float pixelThreshold = 5.0f) const;
    
    // ============================================
    // ✅ v0.3: Gizmo 轴拾取
    // ============================================
    
    /**
     * 拾取 Gizmo 轴
     * @param ray 射线
     * @param document 文档
     * @param threshold 拾取阈值（世界单位）
     * @return 拾取到的轴索引（0=X, 1=Y, 2=Z），如果没有拾取到返回-1
     */
    int pickGizmoAxis(
        const Ray& ray,
        const Document& document,
        float threshold = 0.1f) const;
    
    // ============================================
    // ✅ v0.2: 2D 框选功能
    // ============================================
    
    /**
     * 2D 框选：根据矩形框实体ID，找到所有相交或被包含的实体并设置 hover 状态
     * @param boxEntityId 矩形框实体的 ID
     * @param document 文档（可修改实体的 hovered 状态）
     * @param mode 选择模式（INTERSECT=相交, CONTAIN=完全包含）
     * @return 被选中的实体 ID 列表
     */
    std::vector<EntityId> selectByBox2D(
        EntityId boxEntityId,
        Document& document,
        BoxSelectMode mode = BoxSelectMode::INTERSECT) const;
    
    // ============================================
    // ✅ v0.3: 3D 框选功能（基于工作平面和射线）
    // ============================================
    
    /**
     * 3D 框选：在工作平面上根据矩形框实体ID，使用射线投影方式检测实体
     * @param boxEntityId 矩形框实体的 ID（在工作平面上绘制的框）
     * @param document 文档（可修改实体的 hovered 状态）
     * @param vp 视口状态
     * @param workPlane 工作平面（框所在的平面）
     * @param mode 选择模式（INTERSECT=相交, CONTAIN=完全包含）
     * @return 被选中的实体 ID 列表
     */
    std::vector<EntityId> selectByBox3D(
        EntityId boxEntityId,
        Document& document,
        const ViewportState& vp,
        const class WorkPlane& workPlane,
        BoxSelectMode mode = BoxSelectMode::INTERSECT) const;

private:

    // ============================================
    // 2D 实体距离计算
    // ============================================
    
    // 点到线段的距离
    float distanceToLine2D(
        const glm::vec3& point,
        const Line& line,
        const ViewportState& vp,
        glm::vec3* closestPoint = nullptr) const;
    
    // 点到折线的距离
    float distanceToPolyline2D(
        const glm::vec3& point,
        const Polyline& polyline,
        const ViewportState& vp,
        glm::vec3* closestPoint = nullptr) const;
    
    // 点到圆的距离
    float distanceToCircle2D(
        const glm::vec3& point,
        const Circle& circle,
        const ViewportState& vp,
        glm::vec3* closestPoint = nullptr) const;
    
    // 点到圆弧的距离
    float distanceToArc2D(
        const glm::vec3& point,
        const Arc& arc,
        const ViewportState& vp,
        glm::vec3* closestPoint = nullptr) const;
    
    // 点到立方体的距离
    float distanceToBox2D(
        const glm::vec3& point,
        const Box& box,
        const ViewportState& vp,
        glm::vec3* closestPoint = nullptr) const;
    
    // ============================================
    // 2D 几何辅助方法
    // ============================================
    
    // 点到线段的最短距离（2D）
    static float pointToSegmentDistance(
        const glm::vec2& p,
        const glm::vec2& a,
        const glm::vec2& b,
        glm::vec2* closestPoint = nullptr);
    
    // 世界坐标转屏幕像素距离
    float worldToPixelDistance(
        float worldDist,
        const ViewportState& vp) const {
        return worldDist / vp.worldPerPixel;
    }

    // ============================================
    // 实体求交方法
    // ============================================
    
    // 射线与线段求交
    bool intersectLine(
        const Ray& ray,
        const Line& line,
        glm::vec3& hitPoint,
        float threshold) const;
    
    // 射线与折线求交
    bool intersectPolyline(
        const Ray& ray,
        const Polyline& polyline,
        glm::vec3& hitPoint,
        float threshold) const;
    
    // 射线与圆求交
    bool intersectCircle(
        const Ray& ray,
        const Circle& circle,
        glm::vec3& hitPoint,
        float threshold) const;
    
    // 射线与圆弧求交
    bool intersectArc(
        const Ray& ray,
        const Arc& arc,
        glm::vec3& hitPoint,
        float threshold) const;
    
    // 射线与立方体求交
    bool intersectBox(
        const Ray& ray,
        const Box& box,
        glm::vec3& hitPoint) const;
    
    // ============================================
    // 框选辅助方法
    // ============================================
    
    // 检查点是否在屏幕矩形内
    bool isPointInScreenRect(
        const glm::vec3& worldPoint,
        int minX, int minY, int maxX, int maxY,
        const ViewportState& vp) const;
    
    // 检查线段是否与屏幕矩形相交
    bool isLineIntersectScreenRect(
        const glm::vec3& p0, const glm::vec3& p1,
        int minX, int minY, int maxX, int maxY,
        const ViewportState& vp) const;
    
    // ============================================
    // 2D 框选几何检测辅助方法
    // ============================================
    
    // 检查 2D 点是否在矩形框内
    bool isPoint2DInBox(
        float px, float py,
        float minX, float minY, float maxX, float maxY) const;
    
    // 检查 2D 线段是否与矩形框相交或被包含
    bool checkLineInBox2D(
        const Line& line,
        float minX, float minY, float maxX, float maxY,
        BoxSelectMode mode) const;
    
    // 检查多段线是否与矩形框相交或被包含
    bool checkPolylineInBox2D(
        const Polyline& polyline,
        float minX, float minY, float maxX, float maxY,
        BoxSelectMode mode) const;
    
    // 检查矩形是否与矩形框相交或被包含
    bool checkRectangleInBox2D(
        const Rectangle& rect,
        float minX, float minY, float maxX, float maxY,
        BoxSelectMode mode) const;
    
    // 检查圆是否与矩形框相交或被包含
    bool checkCircleInBox2D(
        const Circle& circle,
        float minX, float minY, float maxX, float maxY,
        BoxSelectMode mode) const;
    
    // 检查圆弧是否与矩形框相交或被包含
    bool checkArcInBox2D(
        const Arc& arc,
        float minX, float minY, float maxX, float maxY,
        BoxSelectMode mode) const;
    
    // 检查 3D 立方体是否与矩形框相交或被包含
    bool checkBox3DInBox2D(
        const Box& box,
        float minX, float minY, float maxX, float maxY,
        BoxSelectMode mode) const;
    
    // 检查 2D 线段是否与矩形框相交
    bool lineSegmentIntersectsBox2D(
        float x0, float y0, float x1, float y1,
        float minX, float minY, float maxX, float maxY) const;
    
    // ============================================
    // 3D 框选几何检测辅助方法（基于射线投影）
    // ============================================
    
    /**
     * 检查实体是否被3D选择框选中（使用射线投影方式）
     * 原理：从框的四个角和若干采样点发射射线，检查射线是否与实体相交
     */
    bool checkEntityInBox3D(
        const Entity& entity,
        const glm::vec3 boxCorners[4],  // 框在工作平面上的四个角点（世界坐标）
        const ViewportState& vp,
        BoxSelectMode mode) const;
    
    /**
     * 生成框选区域的射线网格（从框的边界和内部采样点发射射线）
     * @param boxCorners 框的四个角点（世界坐标）
     * @param vp 视口状态
     * @param sampleCount 每条边的采样数量
     * @return 射线列表
     */
    std::vector<Ray> generateBoxRays(
        const glm::vec3 boxCorners[4],
        const ViewportState& vp,
        int sampleCount = 10) const;
    
    /**
     * 检查点是否在3D框内（工作平面局部坐标系）
     * @param point 世界坐标点
     * @param boxCorners 框的四个角点（世界坐标）
     * @param workPlane 工作平面
     * @return 是否在框内
     */
    bool isPointInBox3D(
        const glm::vec3& point,
        const glm::vec3 boxCorners[4],
        const class WorkPlane& workPlane) const;
};