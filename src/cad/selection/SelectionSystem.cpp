#include "SelectionSystem.h"
#include "../../base/util/WorkPlane.h"
#include "../../base/util/RayUtils.h"
#include <algorithm>
#include <cmath>
#include <QDebug>

SelectionSystem::SelectionSystem(Document* document, QObject* parent)
    : QObject(parent)
    , document_(document)
{
    initializeStyleMapping();
}

void SelectionSystem::initializeStyleMapping() {
    auto& styleMgr = RenderStyleManager::instance();
    
    // 为每种实体类型创建状态到样式的映射
    // LINE类型
    typeStyleMapping_[EntityType::Line][EntityState::NORMAL] = styleMgr.getStyleId("line_normal");
    typeStyleMapping_[EntityType::Line][EntityState::HOVERED] = styleMgr.getStyleId("line_hovered");
    typeStyleMapping_[EntityType::Line][EntityState::SELECTED] = styleMgr.getStyleId("line_selected");
    
    // Polyline类型（使用line样式）
    typeStyleMapping_[EntityType::Polyline] = typeStyleMapping_[EntityType::Line];
    
    // Rectangle类型（使用line样式）
    typeStyleMapping_[EntityType::Rectangle] = typeStyleMapping_[EntityType::Line];
    
    // Circle类型
    typeStyleMapping_[EntityType::Circle][EntityState::NORMAL] = styleMgr.getStyleId("circle_normal");
    typeStyleMapping_[EntityType::Circle][EntityState::HOVERED] = styleMgr.getStyleId("circle_hovered");
    typeStyleMapping_[EntityType::Circle][EntityState::SELECTED] = styleMgr.getStyleId("circle_selected");
    
    // Arc类型（使用circle样式）
    typeStyleMapping_[EntityType::Arc] = typeStyleMapping_[EntityType::Circle];
    
    // Box类型（使用triangle样式）
    typeStyleMapping_[EntityType::Box][EntityState::NORMAL] = styleMgr.getStyleId("triangle_normal");
    typeStyleMapping_[EntityType::Box][EntityState::HOVERED] = styleMgr.getStyleId("triangle_hovered");
    typeStyleMapping_[EntityType::Box][EntityState::SELECTED] = styleMgr.getStyleId("triangle_selected");
    
    // GizmoAxis类型（使用line样式）
    typeStyleMapping_[EntityType::GizmoAxis] = typeStyleMapping_[EntityType::Line];
}

// ============================================
// 几何拾取实现
// ============================================

std::optional<SelectionSystem::PickResult> SelectionSystem::pick2D(
    const glm::vec3& worldPos,
    const ViewportState& vp,
    float pixelThreshold) const {
    
    std::vector<PickResult> results = pickAll2D(worldPos, vp, pixelThreshold);
    
    if (results.empty()) {
        return std::nullopt;
    }
    
    return results.front();
}

// ✅ 统一拾取方法：确保hover和click使用完全相同的逻辑
std::optional<SelectionSystem::PickResult> SelectionSystem::pickUnified(
    const QPoint& screenPos,
    const ViewportState& vp,
    float pixelThreshold) const {
    
    // 统一的坐标转换逻辑
    glm::vec3 worldPos = vp.screenToWorld(screenPos, 0.0f);
    
    // 调试信息：确保坐标转换一致
    // qDebug() << "PickUnified: screen" << screenPos.x() << screenPos.y() << "-> world" << worldPos.x << worldPos.y << worldPos.z;
    
    // 使用相同的拾取算法
    return pick2D(worldPos, vp, pixelThreshold);
}

std::vector<SelectionSystem::PickResult> SelectionSystem::pickAll2D(
    const glm::vec3& worldPos,
    const ViewportState& vp,
    float pixelThreshold) const {
    
    std::vector<PickResult> results;
    
    // ✅ 确保世界阈值计算的稳定性
    float worldThreshold = pixelThreshold * vp.worldPerPixel;
    
    // 添加最小阈值防止过小的拾取区域
    const float minWorldThreshold = 1e-6f;
    worldThreshold = std::max(worldThreshold, minWorldThreshold);
    
    for (const auto* entity : document_->all()) {
        if (!entity) continue;
        
        glm::vec3 closestPoint;
        float distance = std::numeric_limits<float>::max();
        
        switch (entity->type) {
            case EntityType::Line: {
                if (auto* line = std::get_if<Line>(&entity->geom)) {
                    distance = distanceToLine2D(worldPos, *line, vp, &closestPoint);
                }
                break;
            }
            
            case EntityType::Polyline: {
                if (auto* polyline = std::get_if<Polyline>(&entity->geom)) {
                    distance = distanceToPolyline2D(worldPos, *polyline, vp, &closestPoint);
                }
                break;
            }
            
            case EntityType::Circle: {
                if (auto* circle = std::get_if<Circle>(&entity->geom)) {
                    distance = distanceToCircle2D(worldPos, *circle, vp, &closestPoint);
                }
                break;
            }
            
            case EntityType::Arc: {
                if (auto* arc = std::get_if<Arc>(&entity->geom)) {
                    distance = distanceToArc2D(worldPos, *arc, vp, &closestPoint);
                }
                break;
            }
            
            case EntityType::Box: {
                if (auto* box = std::get_if<Box>(&entity->geom)) {
                    distance = distanceToBox2D(worldPos, *box, vp, &closestPoint);
                }
                break;
            }
            
            case EntityType::GizmoAxis: {
                if (auto* gizmo = std::get_if<GizmoAxis>(&entity->geom)) {
                    // ✅ GizmoAxis 拾取：将轴视为一条线段
                    Line axisLine;
                    axisLine.p0 = gizmo->origin;
                    axisLine.p1 = gizmo->origin + gizmo->direction * gizmo->length;
                    distance = distanceToLine2D(worldPos, axisLine, vp, &closestPoint);
                    
                    // ✅ 为 Gizmo 轴增加额外的拾取容忍度，使其更容易选中
                    distance *= 0.3f; // 等效于将拾取范围扩大约3倍
                }
                break;
            }
        }
        
        if (distance <= worldThreshold) {
            results.push_back({entity->id, closestPoint, distance});
        }
    }
    
    // ✅ 稳定排序：确保相同距离的实体有一致的顺序
    std::sort(results.begin(), results.end(), [](const PickResult& a, const PickResult& b) {
        // 首先按距离排序
        const float eps = 1e-6f; // 浮点比较精度
        if (std::abs(a.distance - b.distance) > eps) {
            return a.distance < b.distance;
        }
        // 距离相同时按实体ID排序，确保稳定性
        return a.entityId < b.entityId;
    });
    
    return results;
}

std::vector<EntityId> SelectionSystem::pickByBox2D(
    EntityId boxEntityId,
    BoxSelectMode mode) const {
    
    std::vector<EntityId> selectedIds;
    
    Entity* boxEntity = document_->get(boxEntityId);
    if (!boxEntity || boxEntity->type != EntityType::Rectangle) {
        return selectedIds;
    }
    
    const auto* rect = std::get_if<Rectangle>(&boxEntity->geom);
    if (!rect) return selectedIds;
    
    float minX = std::min(rect->p0.x, rect->p1.x);
    float maxX = std::max(rect->p0.x, rect->p1.x);
    float minY = std::min(rect->p0.y, rect->p1.y);
    float maxY = std::max(rect->p0.y, rect->p1.y);
    
    for (const auto* entity : document_->all()) {
        if (!entity || entity->id == boxEntityId || entity->isGizmo) continue;
        
        bool shouldSelect = false;
        
        switch (entity->type) {
            case EntityType::Line: {
                if (auto* line = std::get_if<Line>(&entity->geom)) {
                    shouldSelect = checkLineInBox2D(*line, minX, minY, maxX, maxY, mode);
                }
                break;
            }
            
            case EntityType::Polyline: {
                if (auto* polyline = std::get_if<Polyline>(&entity->geom)) {
                    shouldSelect = checkPolylineInBox2D(*polyline, minX, minY, maxX, maxY, mode);
                }
                break;
            }
            
            case EntityType::Rectangle: {
                if (auto* rectangle = std::get_if<Rectangle>(&entity->geom)) {
                    shouldSelect = checkRectangleInBox2D(*rectangle, minX, minY, maxX, maxY, mode);
                }
                break;
            }
            
            case EntityType::Circle: {
                if (auto* circle = std::get_if<Circle>(&entity->geom)) {
                    shouldSelect = checkCircleInBox2D(*circle, minX, minY, maxX, maxY, mode);
                }
                break;
            }
            
            case EntityType::Arc: {
                if (auto* arc = std::get_if<Arc>(&entity->geom)) {
                    shouldSelect = checkArcInBox2D(*arc, minX, minY, maxX, maxY, mode);
                }
                break;
            }
            
            case EntityType::Box: {
                if (auto* box = std::get_if<Box>(&entity->geom)) {
                    shouldSelect = checkBox3DInBox2D(*box, minX, minY, maxX, maxY, mode);
                }
                break;
            }
        }
        
        if (shouldSelect) {
            selectedIds.push_back(entity->id);
        }
    }
    
    return selectedIds;
}

std::vector<EntityId> SelectionSystem::pickByBox3D(
    EntityId boxEntityId,
    const ViewportState& vp,
    const WorkPlane& workPlane,
    BoxSelectMode mode) const {
    
    // ✅ 3D模式下的框选：使用屏幕投影的2D逻辑（简化实现）
    // 对于大多数CAD操作，这种方法已经足够
    return pickByBox2D(boxEntityId, mode);
}

int SelectionSystem::pickGizmoAxis(const Ray& ray, float threshold) const {
    int closestAxis = -1;
    float minDistance = threshold;
    
    // 遍历所有 GizmoAxis 实体
    for (const auto* entity : document_->all()) {
        if (!entity || entity->type != EntityType::GizmoAxis) {
            continue;
        }
        
        auto* gizmo = std::get_if<GizmoAxis>(&entity->geom);
        if (!gizmo) continue;
        
        // 计算射线到轴线的距离
        glm::vec3 axisEnd = gizmo->origin + gizmo->direction * gizmo->length;
        
        // 点到线段的最短距离计算
        glm::vec3 v = axisEnd - gizmo->origin;
        glm::vec3 w = ray.getOrigin() - gizmo->origin;
        
        float c1 = glm::dot(w, v);
        float c2 = glm::dot(v, v);
        
        float t = 0.0f;
        if (c2 > 1e-6f) {
            t = glm::clamp(c1 / c2, 0.0f, 1.0f);
        }
        
        glm::vec3 closestPointOnAxis = gizmo->origin + t * v;
        
        // 计算射线上最近点到轴的距离
        glm::vec3 rayDir = ray.getDirection();
        glm::vec3 toAxis = closestPointOnAxis - ray.getOrigin();
        float rayT = glm::dot(toAxis, rayDir) / glm::dot(rayDir, rayDir);
        
        if (rayT > 0.0f) { // 只考虑射线正方向
            glm::vec3 closestPointOnRay = ray.getOrigin() + rayT * rayDir;
            float distance = glm::length(closestPointOnRay - closestPointOnAxis);
            
            if (distance < minDistance) {
                minDistance = distance;
                closestAxis = gizmo->axisIndex;
            }
        }
    }
    
    return closestAxis;
}

// ============================================
// 选择状态管理
// ============================================

void SelectionSystem::clearSelection() {
    if (selectedIds_.empty()) {
        return;
    }
    
    selectedIds_.clear();
    notifySelectionChanged();
}

void SelectionSystem::select(EntityId id) {
    if (!document_->get(id)) {
        qWarning() << "Cannot select non-existent entity:" << id;
        return;
    }
    
    if (selectedIds_.size() == 1 && selectedIds_.count(id) > 0) {
        return;
    }
    
    selectedIds_.clear();
    selectedIds_.insert(id);
    notifySelectionChanged();
}

void SelectionSystem::select(const std::vector<EntityId>& ids) {
    selectedIds_.clear();
    
    for (EntityId id : ids) {
        if (document_->get(id)) {
            selectedIds_.insert(id);
        }
    }
    
    notifySelectionChanged();
}

void SelectionSystem::selectWithMode(EntityId id, SelectMode mode) {
    switch (mode) {
        case SelectMode::REPLACE:
            select(id);
            break;
        case SelectMode::ADD:
            addToSelection(id);
            break;
        case SelectMode::TOGGLE:
            toggleSelection(id);
            break;
    }
}

void SelectionSystem::selectWithMode(const std::vector<EntityId>& ids, SelectMode mode) {
    switch (mode) {
        case SelectMode::REPLACE:
            select(ids);
            break;
        case SelectMode::ADD:
            addToSelection(ids);
            break;
        case SelectMode::TOGGLE:
            for (EntityId id : ids) {
                toggleSelection(id);
            }
            break;
    }
}

void SelectionSystem::addToSelection(EntityId id) {
    if (!document_->get(id)) {
        qWarning() << "Cannot add non-existent entity to selection:" << id;
        return;
    }
    
    if (selectedIds_.count(id) > 0) {
        return;
    }
    
    selectedIds_.insert(id);
    notifySelectionChanged();
}

void SelectionSystem::addToSelection(const std::vector<EntityId>& ids) {
    bool changed = false;
    
    for (EntityId id : ids) {
        if (document_->get(id) && selectedIds_.count(id) == 0) {
            selectedIds_.insert(id);
            changed = true;
        }
    }
    
    if (changed) {
        notifySelectionChanged();
    }
}

void SelectionSystem::removeFromSelection(EntityId id) {
    if (selectedIds_.erase(id) > 0) {
        notifySelectionChanged();
    }
}

void SelectionSystem::removeFromSelection(const std::vector<EntityId>& ids) {
    bool changed = false;
    
    for (EntityId id : ids) {
        if (selectedIds_.erase(id) > 0) {
            changed = true;
        }
    }
    
    if (changed) {
        notifySelectionChanged();
    }
}

void SelectionSystem::toggleSelection(EntityId id) {
    if (!document_->get(id)) {
        return;
    }
    
    if (selectedIds_.count(id) > 0) {
        selectedIds_.erase(id);
    } else {
        selectedIds_.insert(id);
    }
    
    notifySelectionChanged();
}

void SelectionSystem::selectAll() {
    selectedIds_.clear();
    
    for (const auto* entity : document_->all()) {
        if (entity && !entity->isGizmo) {
            selectedIds_.insert(entity->id);
        }
    }
    
    notifySelectionChanged();
}

void SelectionSystem::invertSelection() {
    std::unordered_set<EntityId> newSelection;
    
    for (const auto* entity : document_->all()) {
        if (entity && !entity->isGizmo && selectedIds_.count(entity->id) == 0) {
            newSelection.insert(entity->id);
        }
    }
    
    selectedIds_ = std::move(newSelection);
    notifySelectionChanged();
}

std::vector<Entity*> SelectionSystem::getSelectedEntities() const {
    std::vector<Entity*> entities;
    entities.reserve(selectedIds_.size());
    
    for (EntityId id : selectedIds_) {
        if (Entity* entity = document_->get(id)) {
            entities.push_back(entity);
        }
    }
    
    return entities;
}

// ============================================
// 悬停状态管理
// ============================================

void SelectionSystem::setHovered(EntityId id) {
    if (hoveredIds_.size() == 1 && hoveredIds_.count(id) > 0) {
        return; // 已经是唯一悬停的实体
    }
    
    hoveredIds_.clear();
    hoveredIds_.insert(id);
    emit hoverChanged(id);
}

void SelectionSystem::setHovered(const std::vector<EntityId>& ids) {
    // 检查是否和当前悬停集合相同
    if (ids.size() == hoveredIds_.size()) {
        bool same = true;
        for (EntityId id : ids) {
            if (hoveredIds_.count(id) == 0) {
                same = false;
                break;
            }
        }
        if (same) return; // 悬停集合没有变化
    }
    
    hoveredIds_.clear();
    for (EntityId id : ids) {
        hoveredIds_.insert(id);
    }
    
    // 发出第一个悬停实体的信号（为了向后兼容）
    if (!ids.empty()) {
        emit hoverChanged(ids[0]);
    }
}

void SelectionSystem::clearHovered() {
    if (hoveredIds_.empty()) {
        return;
    }
    
    hoveredIds_.clear();
}

// ============================================
// 样式查询接口
// ============================================

EntityState SelectionSystem::getEntityState(EntityId id) const {
    if (selectedIds_.count(id) > 0) {
        return EntityState::SELECTED;
    }
    
    if (hoveredIds_.count(id) > 0) {
        return EntityState::HOVERED;
    }
    
    return EntityState::NORMAL;
}

StyleId SelectionSystem::getStyleIdForEntity(const Entity* entity) const {
    if (!entity) return 0;
    
    EntityState state = getEntityState(entity->id);
    
    auto typeIt = typeStyleMapping_.find(entity->type);
    if (typeIt == typeStyleMapping_.end()) {
        return 0;
    }
    
    auto stateIt = typeIt->second.find(state);
    if (stateIt == typeIt->second.end()) {
        return 0;
    }
    
    return stateIt->second;
}

// ============================================
// 内部辅助方法
// ============================================

void SelectionSystem::notifySelectionChanged() {
    emit selectionChanged(static_cast<int>(selectedIds_.size()));
    
    std::vector<EntityId> ids(selectedIds_.begin(), selectedIds_.end());
    emit selectedEntitiesChanged(ids);
}

// ============================================
// 2D 距离计算实现
// ============================================

float SelectionSystem::distanceToLine2D(
    const glm::vec3& point,
    const Line& line,
    const ViewportState& vp,
    glm::vec3* closestPoint) const {
    
    glm::vec2 p = vp.worldToScreen(point);
    glm::vec2 a = vp.worldToScreen(line.p0);
    glm::vec2 b = vp.worldToScreen(line.p1);
    
    glm::vec2 closest2D;
    float pixelDist = pointToSegmentDistance(p, a, b, &closest2D);
    
    if (closestPoint) {
        *closestPoint = vp.screenToWorld(
            static_cast<int>(closest2D.x),
            static_cast<int>(closest2D.y),
            point.z
        );
    }
    
    return pixelDist * vp.worldPerPixel;
}

float SelectionSystem::distanceToPolyline2D(
    const glm::vec3& point,
    const Polyline& polyline,
    const ViewportState& vp,
    glm::vec3* closestPoint) const {
    
    if (polyline.pts.size() < 2) {
        return std::numeric_limits<float>::max();
    }
    
    glm::vec2 p = vp.worldToScreen(point);
    
    float minDist = std::numeric_limits<float>::max();
    glm::vec2 closestPt2D;
    
    auto checkSegment = [&](const glm::vec3& pt0, const glm::vec3& pt1) {
        glm::vec2 a = vp.worldToScreen(pt0);
        glm::vec2 b = vp.worldToScreen(pt1);
        
        glm::vec2 tempClosest;
        float dist = pointToSegmentDistance(p, a, b, &tempClosest);
        
        if (dist < minDist) {
            minDist = dist;
            closestPt2D = tempClosest;
        }
    };
    
    for (size_t i = 0; i < polyline.pts.size() - 1; ++i) {
        checkSegment(polyline.pts[i], polyline.pts[i + 1]);
    }
    
    if (polyline.closed && polyline.pts.size() > 2) {
        checkSegment(polyline.pts.back(), polyline.pts.front());
    }
    
    if (closestPoint) {
        *closestPoint = vp.screenToWorld(
            static_cast<int>(closestPt2D.x),
            static_cast<int>(closestPt2D.y),
            point.z
        );
    }
    
    return minDist * vp.worldPerPixel;
}

float SelectionSystem::distanceToCircle2D(
    const glm::vec3& point,
    const Circle& circle,
    const ViewportState& vp,
    glm::vec3* closestPoint) const {
    
    glm::vec2 p = vp.worldToScreen(point);
    glm::vec2 center = vp.worldToScreen(circle.c);
    
    glm::vec3 circleEdgePoint = circle.c + glm::vec3(circle.r, 0.0f, 0.0f);
    glm::vec2 edgeScreen = vp.worldToScreen(circleEdgePoint);
    
    float radiusPixel = glm::distance(center, edgeScreen);
    float distToCenter = glm::distance(p, center);
    float distToCirclePixel = std::abs(distToCenter - radiusPixel);
    
    if (closestPoint) {
        if (distToCenter > 1e-6f) {
            glm::vec2 direction = glm::normalize(p - center);
            glm::vec2 closest2D = center + direction * radiusPixel;
            
            *closestPoint = vp.screenToWorld(
                static_cast<int>(closest2D.x),
                static_cast<int>(closest2D.y),
                circle.c.z
            );
        } else {
            *closestPoint = circleEdgePoint;
        }
    }
    
    float pixelSizeAtCenter = vp.getPixelSizeAt(circle.c);
    return distToCirclePixel * pixelSizeAtCenter;
}

float SelectionSystem::distanceToArc2D(
    const glm::vec3& point,
    const Arc& arc,
    const ViewportState& vp,
    glm::vec3* closestPoint) const {
    
    glm::vec2 p2D(point.x, point.y);
    glm::vec2 center2D(arc.c.x, arc.c.y);
    
    glm::vec2 toPoint = p2D - center2D;
    float angle = std::atan2(toPoint.y, toPoint.x);
    
    if (angle < 0) angle += 2.0f * glm::pi<float>();
    
    float a0 = arc.a0;
    float a1 = arc.a1;
    
    bool inArc = false;
    if (a0 <= a1) {
        inArc = (angle >= a0 && angle <= a1);
    } else {
        inArc = (angle >= a0 || angle <= a1);
    }
    
    if (inArc) {
        float distToCenter = glm::distance(p2D, center2D);
        float distToArc = std::abs(distToCenter - arc.r);
        
        if (closestPoint) {
            if (glm::length(toPoint) > 1e-6f) {
                glm::vec2 direction = glm::normalize(toPoint);
                glm::vec2 closest2D = center2D + direction * arc.r;
                *closestPoint = glm::vec3(closest2D.x, closest2D.y, arc.c.z);
            } else {
                *closestPoint = glm::vec3(arc.c.x + arc.r, arc.c.y, arc.c.z);
            }
        }
        
        return distToArc;
    } else {
        glm::vec2 p0(
            center2D.x + arc.r * std::cos(a0),
            center2D.y + arc.r * std::sin(a0)
        );
        glm::vec2 p1(
            center2D.x + arc.r * std::cos(a1),
            center2D.y + arc.r * std::sin(a1)
        );
        
        float dist0 = glm::distance(p2D, p0);
        float dist1 = glm::distance(p2D, p1);
        
        if (dist0 < dist1) {
            if (closestPoint) {
                *closestPoint = glm::vec3(p0.x, p0.y, arc.c.z);
            }
            return dist0;
        } else {
            if (closestPoint) {
                *closestPoint = glm::vec3(p1.x, p1.y, arc.c.z);
            }
            return dist1;
        }
    }
}

float SelectionSystem::distanceToBox2D(
    const glm::vec3& point,
    const Box& box,
    const ViewportState& vp,
    glm::vec3* closestPoint) const {
    
    float half = box.size * 0.5f;
    glm::vec3 c = box.center;
    
    glm::vec3 vertices[8] = {
        c + glm::vec3(-half, -half, -half),
        c + glm::vec3( half, -half, -half),
        c + glm::vec3( half,  half, -half),
        c + glm::vec3(-half,  half, -half),
        c + glm::vec3(-half, -half,  half),
        c + glm::vec3( half, -half,  half),
        c + glm::vec3( half,  half,  half),
        c + glm::vec3(-half,  half,  half),
    };
    
    int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},
        {4, 5}, {5, 6}, {6, 7}, {7, 4},
        {0, 4}, {1, 5}, {2, 6}, {3, 7}
    };
    
    glm::vec2 p = vp.worldToScreen(point);
    
    float minDist = std::numeric_limits<float>::max();
    glm::vec2 closestPt2D;
    
    for (int i = 0; i < 12; ++i) {
        glm::vec2 a = vp.worldToScreen(vertices[edges[i][0]]);
        glm::vec2 b = vp.worldToScreen(vertices[edges[i][1]]);
        
        glm::vec2 tempClosest;
        float dist = pointToSegmentDistance(p, a, b, &tempClosest);
        
        if (dist < minDist) {
            minDist = dist;
            closestPt2D = tempClosest;
        }
    }
    
    if (closestPoint) {
        *closestPoint = vp.screenToWorld(
            static_cast<int>(closestPt2D.x),
            static_cast<int>(closestPt2D.y),
            point.z
        );
    }
    
    return minDist * vp.worldPerPixel;
}

float SelectionSystem::pointToSegmentDistance(
    const glm::vec2& p,
    const glm::vec2& a,
    const glm::vec2& b,
    glm::vec2* closestPoint) {
    
    glm::vec2 ab = b - a;
    glm::vec2 ap = p - a;
    
    float abLenSq = glm::dot(ab, ab);
    
    if (abLenSq < 1e-6f) {
        if (closestPoint) {
            *closestPoint = a;
        }
        return glm::distance(p, a);
    }
    
    float t = glm::dot(ap, ab) / abLenSq;
    t = glm::clamp(t, 0.0f, 1.0f);
    
    glm::vec2 closest = a + t * ab;
    
    if (closestPoint) {
        *closestPoint = closest;
    }
    
    return glm::distance(p, closest);
}

// ============================================
// 框选辅助方法实现
// ============================================

bool SelectionSystem::isPoint2DInBox(
    float px, float py,
    float minX, float minY, float maxX, float maxY) const {
    return px >= minX && px <= maxX && py >= minY && py <= maxY;
}

bool SelectionSystem::checkLineInBox2D(
    const Line& line,
    float minX, float minY, float maxX, float maxY,
    BoxSelectMode mode) const {
    
    bool p0In = isPoint2DInBox(line.p0.x, line.p0.y, minX, minY, maxX, maxY);
    bool p1In = isPoint2DInBox(line.p1.x, line.p1.y, minX, minY, maxX, maxY);
    
    if (mode == BoxSelectMode::CONTAIN) {
        return p0In && p1In;
    } else {
        return p0In || p1In || 
               lineSegmentIntersectsBox2D(line.p0.x, line.p0.y, line.p1.x, line.p1.y,
                                         minX, minY, maxX, maxY);
    }
}

bool SelectionSystem::checkPolylineInBox2D(
    const Polyline& polyline,
    float minX, float minY, float maxX, float maxY,
    BoxSelectMode mode) const {
    
    if (polyline.pts.empty()) {
        return false;
    }
    
    if (mode == BoxSelectMode::CONTAIN) {
        for (const auto& pt : polyline.pts) {
            if (!isPoint2DInBox(pt.x, pt.y, minX, minY, maxX, maxY)) {
                return false;
            }
        }
        return true;
    } else {
        for (const auto& pt : polyline.pts) {
            if (isPoint2DInBox(pt.x, pt.y, minX, minY, maxX, maxY)) {
                return true;
            }
        }
        
        for (size_t i = 0; i < polyline.pts.size() - 1; ++i) {
            const auto& p0 = polyline.pts[i];
            const auto& p1 = polyline.pts[i + 1];
            if (lineSegmentIntersectsBox2D(p0.x, p0.y, p1.x, p1.y,
                                          minX, minY, maxX, maxY)) {
                return true;
            }
        }
        
        if (polyline.closed && polyline.pts.size() > 2) {
            const auto& p0 = polyline.pts.back();
            const auto& p1 = polyline.pts.front();
            if (lineSegmentIntersectsBox2D(p0.x, p0.y, p1.x, p1.y,
                                          minX, minY, maxX, maxY)) {
                return true;
            }
        }
        
        return false;
    }
}

bool SelectionSystem::checkRectangleInBox2D(
    const Rectangle& rect,
    float minX, float minY, float maxX, float maxY,
    BoxSelectMode mode) const {
    
    float rectMinX = std::min(rect.p0.x, rect.p1.x);
    float rectMaxX = std::max(rect.p0.x, rect.p1.x);
    float rectMinY = std::min(rect.p0.y, rect.p1.y);
    float rectMaxY = std::max(rect.p0.y, rect.p1.y);
    
    if (mode == BoxSelectMode::CONTAIN) {
        return rectMinX >= minX && rectMaxX <= maxX &&
               rectMinY >= minY && rectMaxY <= maxY;
    } else {
        return !(rectMaxX < minX || rectMinX > maxX ||
                 rectMaxY < minY || rectMinY > maxY);
    }
}

bool SelectionSystem::checkCircleInBox2D(
    const Circle& circle,
    float minX, float minY, float maxX, float maxY,
    BoxSelectMode mode) const {
    
    float cx = circle.c.x;
    float cy = circle.c.y;
    float r = circle.r;
    
    if (mode == BoxSelectMode::CONTAIN) {
        return (cx - r >= minX) && (cx + r <= maxX) &&
               (cy - r >= minY) && (cy + r <= maxY);
    } else {
        float closestX = std::clamp(cx, minX, maxX);
        float closestY = std::clamp(cy, minY, maxY);
        
        float dx = cx - closestX;
        float dy = cy - closestY;
        float distSq = dx * dx + dy * dy;
        
        return distSq <= (r * r);
    }
}

bool SelectionSystem::checkArcInBox2D(
    const Arc& arc,
    float minX, float minY, float maxX, float maxY,
    BoxSelectMode mode) const {
    
    float cx = arc.c.x;
    float cy = arc.c.y;
    float r = arc.r;
    
    const int samples = 16;
    float angleRange = arc.a1 - arc.a0;
    if (angleRange < 0) angleRange += 2.0f * glm::pi<float>();
    
    if (mode == BoxSelectMode::CONTAIN) {
        for (int i = 0; i <= samples; ++i) {
            float t = float(i) / samples;
            float angle = arc.a0 + t * angleRange;
            float px = cx + r * std::cos(angle);
            float py = cy + r * std::sin(angle);
            
            if (!isPoint2DInBox(px, py, minX, minY, maxX, maxY)) {
                return false;
            }
        }
        return true;
    } else {
        for (int i = 0; i <= samples; ++i) {
            float t = float(i) / samples;
            float angle = arc.a0 + t * angleRange;
            float px = cx + r * std::cos(angle);
            float py = cy + r * std::sin(angle);
            
            if (isPoint2DInBox(px, py, minX, minY, maxX, maxY)) {
                return true;
            }
        }
        
        return false;
    }
}

bool SelectionSystem::checkBox3DInBox2D(
    const Box& box,
    float minX, float minY, float maxX, float maxY,
    BoxSelectMode mode) const {
    
    float half = box.size * 0.5f;
    glm::vec3 c = box.center;
    
    glm::vec3 vertices[8] = {
        c + glm::vec3(-half, -half, -half),
        c + glm::vec3( half, -half, -half),
        c + glm::vec3( half,  half, -half),
        c + glm::vec3(-half,  half, -half),
        c + glm::vec3(-half, -half,  half),
        c + glm::vec3( half, -half,  half),
        c + glm::vec3( half,  half,  half),
        c + glm::vec3(-half,  half,  half),
    };
    
    if (mode == BoxSelectMode::CONTAIN) {
        for (int i = 0; i < 8; ++i) {
            if (!isPoint2DInBox(vertices[i].x, vertices[i].y, minX, minY, maxX, maxY)) {
                return false;
            }
        }
        return true;
    } else {
        for (int i = 0; i < 8; ++i) {
            if (isPoint2DInBox(vertices[i].x, vertices[i].y, minX, minY, maxX, maxY)) {
                return true;
            }
        }
        
        float boxMinX = vertices[0].x, boxMaxX = vertices[0].x;
        float boxMinY = vertices[0].y, boxMaxY = vertices[0].y;
        
        for (int i = 1; i < 8; ++i) {
            boxMinX = std::min(boxMinX, vertices[i].x);
            boxMaxX = std::max(boxMaxX, vertices[i].x);
            boxMinY = std::min(boxMinY, vertices[i].y);
            boxMaxY = std::max(boxMaxY, vertices[i].y);
        }
        
        return !(boxMaxX < minX || boxMinX > maxX ||
                 boxMaxY < minY || boxMinY > maxY);
    }
}

bool SelectionSystem::lineSegmentIntersectsBox2D(
    float x0, float y0, float x1, float y1,
    float minX, float minY, float maxX, float maxY) const {
    
    auto computeCode = [&](float x, float y) -> int {
        int code = 0;
        if (x < minX) code |= 1;
        if (x > maxX) code |= 2;
        if (y < minY) code |= 4;
        if (y > maxY) code |= 8;
        return code;
    };
    
    int code0 = computeCode(x0, y0);
    int code1 = computeCode(x1, y1);
    
    if (code0 == 0 || code1 == 0) {
        return true;
    }
    
    if ((code0 & code1) != 0) {
        return false;
    }
    
    return true;
}
