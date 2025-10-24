#include "Picker.h"
#include <QDebug>
#include <algorithm>
#include <cmath>

// ============================================
// 单点拾取（射线）
// ============================================

std::optional<Picker::PickResult> Picker::pick(
    int screenX, int screenY,
    const Document& document,
    const ViewportState& vp,
    float threshold) const {
    
    // 生成射线
    Ray ray = Ray::fromScreen(screenX, screenY, vp.width, vp.height, vp.view, vp.proj);
    
    return pick(ray, document, threshold);
}

std::optional<Picker::PickResult> Picker::pick(
    const Ray& ray,
    const Document& document,
    float threshold) const {
    
    std::vector<PickResult> results = pickAll(ray, document, threshold);
    
    if (results.empty()) {
        return std::nullopt;
    }
    
    // 返回距离最近的
    return results.front();
}

std::vector<Picker::PickResult> Picker::pickAll(
    const Ray& ray,
    const Document& document,
    float threshold) const {
    
    std::vector<PickResult> results;
    
    for (const auto* entity : document.all()) {
        if (!entity) continue;
        
        glm::vec3 hitPoint;
        bool hit = false;
        
        switch (entity->type) {
            case EntityType::Line: {
                if (auto* line = std::get_if<Line>(&entity->geom)) {
                    hit = intersectLine(ray, *line, hitPoint, threshold);
                }
                break;
            }
            
            case EntityType::Polyline: {
                if (auto* polyline = std::get_if<Polyline>(&entity->geom)) {
                    hit = intersectPolyline(ray, *polyline, hitPoint, threshold);
                }
                break;
            }
            
            case EntityType::Circle: {
                if (auto* circle = std::get_if<Circle>(&entity->geom)) {
                    hit = intersectCircle(ray, *circle, hitPoint, threshold);
                }
                break;
            }
            
            case EntityType::Arc: {
                if (auto* arc = std::get_if<Arc>(&entity->geom)) {
                    hit = intersectArc(ray, *arc, hitPoint, threshold);
                }
                break;
            }
            
            case EntityType::Box: {
                if (auto* box = std::get_if<Box>(&entity->geom)) {
                    hit = intersectBox(ray, *box, hitPoint);
                }
                break;
            }
        }
        
        if (hit) {
            float distance = glm::distance(ray.getOrigin(), hitPoint);
            results.push_back({entity->id, hitPoint, distance});
        }
    }
    
    // 按距离排序
    std::sort(results.begin(), results.end());
    
    return results;
}

// ============================================
// 实体求交实现
// ============================================

bool Picker::intersectLine(
    const Ray& ray,
    const Line& line,
    glm::vec3& hitPoint,
    float threshold) const {
    
    return ray.intersectLineSegment(line.p0, line.p1, hitPoint, threshold);
}

bool Picker::intersectPolyline(
    const Ray& ray,
    const Polyline& polyline,
    glm::vec3& hitPoint,
    float threshold) const {
    
    if (polyline.pts.size() < 2) {
        return false;
    }
    
    // 检查所有线段
    float minDist = std::numeric_limits<float>::max();
    glm::vec3 closestPoint;
    bool found = false;
    
    for (size_t i = 0; i < polyline.pts.size() - 1; ++i) {
        glm::vec3 tempHit;
        if (ray.intersectLineSegment(polyline.pts[i], polyline.pts[i + 1], tempHit, threshold)) {
            float dist = glm::distance(ray.getOrigin(), tempHit);
            if (dist < minDist) {
                minDist = dist;
                closestPoint = tempHit;
                found = true;
            }
        }
    }
    
    if (found) {
        hitPoint = closestPoint;
    }
    
    return found;
}

bool Picker::intersectCircle(
    const Ray& ray,
    const Circle& circle,
    glm::vec3& hitPoint,
    float threshold) const {
    
    // 简化：将圆视为球体与射线求交
    // 更精确的方法需要考虑圆所在平面
    
    // 1. 找到射线与圆平面的交点
    glm::vec3 planeNormal(0, 0, 1);  // 假设圆在 XY 平面
    glm::vec3 planeIntersection;
    
    if (!ray.intersectPlane(circle.c, planeNormal, planeIntersection)) {
        return false;
    }
    
    // 2. 检查交点到圆心的距离
    float dist = glm::distance(glm::vec2(planeIntersection.x, planeIntersection.y),
                               glm::vec2(circle.c.x, circle.c.y));
    
    // 在圆周附近（带阈值）
    if (std::abs(dist - circle.r) < threshold) {
        hitPoint = planeIntersection;
        return true;
    }
    
    return false;
}

bool Picker::intersectArc(
    const Ray& ray,
    const Arc& arc,
    glm::vec3& hitPoint,
    float threshold) const {
    
    // 简化实现：先当作完整圆处理，再检查角度范围
    Circle tempCircle{arc.c, arc.r};
    
    if (!intersectCircle(ray, tempCircle, hitPoint, threshold)) {
        return false;
    }
    
    // TODO: 检查交点是否在圆弧的角度范围内
    // 计算交点相对于圆心的角度，判断是否在 [a0, a1] 范围内
    
    return true;  // 暂时简化
}

bool Picker::intersectBox(
    const Ray& ray,
    const Box& box,
    glm::vec3& hitPoint) const {
    
    float half = box.size * 0.5f;
    glm::vec3 boxMin = box.center - glm::vec3(half);
    glm::vec3 boxMax = box.center + glm::vec3(half);
    
    return ray.intersectAABB(boxMin, boxMax, hitPoint);
}

// ============================================
// 框选拾取
// ============================================

std::vector<EntityId> Picker::pickBox(
    int minX, int minY,
    int maxX, int maxY,
    const Document& document,
    const ViewportState& vp,
    BoxSelectMode mode) const {
    
    // 确保 min < max
    if (minX > maxX) std::swap(minX, maxX);
    if (minY > maxY) std::swap(minY, maxY);
    
    std::vector<EntityId> selectedIds;
    
    for (const auto* entity : document.all()) {
        if (!entity) continue;
        
        bool shouldSelect = false;
        
        // 根据实体类型检查是否在框内
        switch (entity->type) {
            case EntityType::Line: {
                if (auto* line = std::get_if<Line>(&entity->geom)) {
                    bool p0In = isPointInScreenRect(line->p0, minX, minY, maxX, maxY, vp);
                    bool p1In = isPointInScreenRect(line->p1, minX, minY, maxX, maxY, vp);
                    
                    if (mode == BoxSelectMode::CONTAIN) {
                        shouldSelect = p0In && p1In;
                    } else {
                        shouldSelect = p0In || p1In || 
                                     isLineIntersectScreenRect(line->p0, line->p1, 
                                                              minX, minY, maxX, maxY, vp);
                    }
                }
                break;
            }
            
            case EntityType::Polyline: {
                if (auto* polyline = std::get_if<Polyline>(&entity->geom)) {
                    if (mode == BoxSelectMode::CONTAIN) {
                        // 所有点都在框内
                        shouldSelect = true;
                        for (const auto& pt : polyline->pts) {
                            if (!isPointInScreenRect(pt, minX, minY, maxX, maxY, vp)) {
                                shouldSelect = false;
                                break;
                            }
                        }
                    } else {
                        // 任意点在框内或任意线段相交
                        for (const auto& pt : polyline->pts) {
                            if (isPointInScreenRect(pt, minX, minY, maxX, maxY, vp)) {
                                shouldSelect = true;
                                break;
                            }
                        }
                    }
                }
                break;
            }
            
            case EntityType::Circle: {
                if (auto* circle = std::get_if<Circle>(&entity->geom)) {
                    // 简化：只检查圆心
                    shouldSelect = isPointInScreenRect(circle->c, minX, minY, maxX, maxY, vp);
                }
                break;
            }
            
            case EntityType::Arc: {
                if (auto* arc = std::get_if<Arc>(&entity->geom)) {
                    shouldSelect = isPointInScreenRect(arc->c, minX, minY, maxX, maxY, vp);
                }
                break;
            }
            
            case EntityType::Box: {
                if (auto* box = std::get_if<Box>(&entity->geom)) {
                    shouldSelect = isPointInScreenRect(box->center, minX, minY, maxX, maxY, vp);
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

// ============================================
// 框选辅助方法
// ============================================

bool Picker::isPointInScreenRect(
    const glm::vec3& worldPoint,
    int minX, int minY, int maxX, int maxY,
    const ViewportState& vp) const {
    
    glm::vec2 screenPos = vp.worldToScreen(worldPoint);
    
    return screenPos.x >= minX && screenPos.x <= maxX &&
           screenPos.y >= minY && screenPos.y <= maxY;
}

bool Picker::isLineIntersectScreenRect(
    const glm::vec3& p0, const glm::vec3& p1,
    int minX, int minY, int maxX, int maxY,
    const ViewportState& vp) const {
    
    // 简化：使用 Cohen-Sutherland 线段裁剪算法
    // 这里先用简单方法：检查线段的包围盒是否与矩形相交
    
    glm::vec2 s0 = vp.worldToScreen(p0);
    glm::vec2 s1 = vp.worldToScreen(p1);
    
    float lineMinX = std::min(s0.x, s1.x);
    float lineMaxX = std::max(s0.x, s1.x);
    float lineMinY = std::min(s0.y, s1.y);
    float lineMaxY = std::max(s0.y, s1.y);
    
    // AABB 相交检测
    return !(lineMaxX < minX || lineMinX > maxX ||
             lineMaxY < minY || lineMinY > maxY);
}