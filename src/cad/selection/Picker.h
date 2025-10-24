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

private:
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
};