#pragma once
#include "../data/document.h"
#include "../data/renderer.h" // for ViewportState
#include "../../base/util/RayUtils.h"
#include "../../base/util/RenderStyleManager.h"
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <optional>
#include <QObject>

/**
 * SelectionSystem - 统一选择系统
 * 
 * 整合了Picker和SelectionManager的功能：
 * - 几何拾取（射线拾取、框选）
 * - 选择状态管理（单选、多选、全选等）
 * - 实体状态跟踪（normal/hovered/selected）
 * - 与RenderStyleManager配合，提供样式查询
 */
class SelectionSystem : public QObject {
    Q_OBJECT

public:
    /**
     * 拾取结果
     */
    struct PickResult {
        EntityId entityId;
        glm::vec3 hitPoint;
        float distance;
        
        bool operator<(const PickResult& other) const {
            return distance < other.distance;
        }
    };
    
    /**
     * 框选模式
     */
    enum class BoxSelectMode {
        INTERSECT,  // 只要相交就选中
        CONTAIN     // 必须完全包含
    };
    
    /**
     * 选择模式
     */
    enum class SelectMode {
        REPLACE,    // 替换选择（默认）
        ADD,        // 添加到选择（Shift）
        TOGGLE      // 切换选择（Ctrl）
    };

public:
    explicit SelectionSystem(Document* document, QObject* parent = nullptr);

    // ============================================
    // 几何拾取接口
    // ============================================
    
    /**
     * 2D 模式下的拾取（直接使用世界坐标）
     */
    std::optional<PickResult> pick2D(
        const glm::vec3& worldPos,
        const ViewportState& vp,
        float pixelThreshold = 5.0f) const;
    
    /**
     * 2D 模式下拾取所有实体
     */
    std::vector<PickResult> pickAll2D(
        const glm::vec3& worldPos,
        const ViewportState& vp,
        float pixelThreshold = 5.0f) const;
    
    /**
     * 2D 框选：根据矩形框实体ID，找到所有相交或被包含的实体
     */
    std::vector<EntityId> pickByBox2D(
        EntityId boxEntityId,
        BoxSelectMode mode = BoxSelectMode::INTERSECT) const;
    
    /**
     * 3D 框选（预留接口）
     */
    std::vector<EntityId> pickByBox3D(
        EntityId boxEntityId,
        const ViewportState& vp,
        const class WorkPlane& workPlane,
        BoxSelectMode mode = BoxSelectMode::INTERSECT) const;

    // ============================================
    // 选择状态管理
    // ============================================
    
    /**
     * 清空选择
     */
    void clearSelection();
    
    /**
     * 选中单个/多个实体
     */
    void select(EntityId id);
    void select(const std::vector<EntityId>& ids);
    
    /**
     * 根据模式选择
     */
    void selectWithMode(EntityId id, SelectMode mode);
    void selectWithMode(const std::vector<EntityId>& ids, SelectMode mode);
    
    /**
     * 添加到选择
     */
    void addToSelection(EntityId id);
    void addToSelection(const std::vector<EntityId>& ids);
    
    /**
     * 从选择中移除
     */
    void removeFromSelection(EntityId id);
    void removeFromSelection(const std::vector<EntityId>& ids);
    
    /**
     * 切换选择状态
     */
    void toggleSelection(EntityId id);
    
    /**
     * 全选/反选
     */
    void selectAll();
    void invertSelection();
    
    /**
     * 查询选择状态
     */
    bool isSelected(EntityId id) const {
        return selectedIds_.count(id) > 0;
    }
    
    size_t getSelectionCount() const {
        return selectedIds_.size();
    }
    
    bool hasSelection() const {
        return !selectedIds_.empty();
    }
    
    const std::unordered_set<EntityId>& getSelectedIds() const {
        return selectedIds_;
    }
    
    std::vector<Entity*> getSelectedEntities() const;

    // ============================================
    // 悬停状态管理
    // ============================================
    
    /**
     * 设置悬停实体（单个）
     */
    void setHovered(EntityId id);
    
    /**
     * 设置悬停实体（多个）
     */
    void setHovered(const std::vector<EntityId>& ids);
    
    /**
     * 清空悬停
     */
    void clearHovered();
    
    /**
     * 查询悬停状态
     */
    bool isHovered(EntityId id) const {
        return hoveredIds_.count(id) > 0;
    }
    
    /**
     * 获取悬停的实体ID集合
     */
    const std::unordered_set<EntityId>& getHoveredIds() const {
        return hoveredIds_;
    }
    
    /**
     * @deprecated 使用 getHoveredIds() 代替
     * 为了向后兼容，返回第一个悬停的实体
     */
    std::optional<EntityId> getHoveredId() const {
        if (hoveredIds_.empty()) {
            return std::nullopt;
        }
        return *hoveredIds_.begin();
    }

    // ============================================
    // 样式查询接口（供Renderer使用）
    // ============================================
    
    /**
     * 获取实体的渲染状态
     */
    EntityState getEntityState(EntityId id) const;
    
    /**
     * 获取实体应使用的样式ID
     * @param entity 实体指针
     * @return 样式ID，如果实体类型未映射则返回0
     */
    StyleId getStyleIdForEntity(const Entity* entity) const;

signals:
    /**
     * 选择变化信号
     */
    void selectionChanged(int selectedCount);
    void selectedEntitiesChanged(const std::vector<EntityId>& ids);
    
    /**
     * 悬停变化信号
     */
    void hoverChanged(EntityId hoveredId);

private:
    // ============================================
    // 内部辅助方法
    // ============================================
    
    void notifySelectionChanged();
    
    // 2D 实体距离计算
    float distanceToLine2D(
        const glm::vec3& point,
        const Line& line,
        const ViewportState& vp,
        glm::vec3* closestPoint = nullptr) const;
    
    float distanceToPolyline2D(
        const glm::vec3& point,
        const Polyline& polyline,
        const ViewportState& vp,
        glm::vec3* closestPoint = nullptr) const;
    
    float distanceToCircle2D(
        const glm::vec3& point,
        const Circle& circle,
        const ViewportState& vp,
        glm::vec3* closestPoint = nullptr) const;
    
    float distanceToArc2D(
        const glm::vec3& point,
        const Arc& arc,
        const ViewportState& vp,
        glm::vec3* closestPoint = nullptr) const;
    
    float distanceToBox2D(
        const glm::vec3& point,
        const Box& box,
        const ViewportState& vp,
        glm::vec3* closestPoint = nullptr) const;
    
    // 2D 几何辅助
    static float pointToSegmentDistance(
        const glm::vec2& p,
        const glm::vec2& a,
        const glm::vec2& b,
        glm::vec2* closestPoint = nullptr);
    
    float worldToPixelDistance(float worldDist, const ViewportState& vp) const {
        return worldDist / vp.worldPerPixel;
    }
    
    // 框选辅助
    bool isPoint2DInBox(
        float px, float py,
        float minX, float minY, float maxX, float maxY) const;
    
    bool checkLineInBox2D(
        const Line& line,
        float minX, float minY, float maxX, float maxY,
        BoxSelectMode mode) const;
    
    bool checkPolylineInBox2D(
        const Polyline& polyline,
        float minX, float minY, float maxX, float maxY,
        BoxSelectMode mode) const;
    
    bool checkRectangleInBox2D(
        const Rectangle& rect,
        float minX, float minY, float maxX, float maxY,
        BoxSelectMode mode) const;
    
    bool checkCircleInBox2D(
        const Circle& circle,
        float minX, float minY, float maxX, float maxY,
        BoxSelectMode mode) const;
    
    bool checkArcInBox2D(
        const Arc& arc,
        float minX, float minY, float maxX, float maxY,
        BoxSelectMode mode) const;
    
    bool checkBox3DInBox2D(
        const Box& box,
        float minX, float minY, float maxX, float maxY,
        BoxSelectMode mode) const;
    
    bool lineSegmentIntersectsBox2D(
        float x0, float y0, float x1, float y1,
        float minX, float minY, float maxX, float maxY) const;

private:
    Document* document_;
    std::unordered_set<EntityId> selectedIds_;  // 选中的实体ID集合
    std::unordered_set<EntityId> hoveredIds_;   // 当前悬停的实体ID集合（支持多实体悬停）
    
    // 实体类型到样式的映射（状态相关）
    // 使用 EntityType 作为 key，值是一个 map：EntityState -> StyleId
    std::unordered_map<EntityType, std::unordered_map<EntityState, StyleId>> typeStyleMapping_;
    
    void initializeStyleMapping();
};
