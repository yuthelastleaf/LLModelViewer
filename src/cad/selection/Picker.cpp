#include "Picker.h"
#include "../../base/util/WorkPlane.h"
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

// ============================================
// 2D 拾取实现
// ============================================

std::optional<Picker::PickResult> Picker::pick2D(
    const glm::vec3& worldPos,
    const Document& document,
    const ViewportState& vp,
    float pixelThreshold) const {
    
    std::vector<PickResult> results = pickAll2D(worldPos, document, vp, pixelThreshold);
    
    if (results.empty()) {
        return std::nullopt;
    }
    
    // 返回距离最近的
    return results.front();
}

std::vector<Picker::PickResult> Picker::pickAll2D(
    const glm::vec3& worldPos,
    const Document& document,
    const ViewportState& vp,
    float pixelThreshold) const {
    
    std::vector<PickResult> results;
    
    // 将像素阈值转换为世界单位
    float worldThreshold = pixelThreshold * vp.worldPerPixel;
    
    for (const auto* entity : document.all()) {
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
        }
        
        // 检查是否在阈值内
        if (distance <= worldThreshold) {
            results.push_back({entity->id, closestPoint, distance});
        }
    }
    
    std::sort(results.begin(), results.end());
    
    return results;
}

// ============================================
// ✅ v0.3: Gizmo 轴拾取实现
// ============================================

int Picker::pickGizmoAxis(
    const Ray& ray,
    const Document& document,
    float threshold) const {
    
    int pickedAxis = -1;
    float minDistance = std::numeric_limits<float>::max();
    
    // 遍历所有 GizmoAxis 实体
    for (const auto* entity : document.all()) {
        if (!entity || entity->type != EntityType::GizmoAxis) continue;
        if (!entity->isGizmo) continue;  // 确保是 Gizmo 实体
        
        const auto* gizmo = std::get_if<GizmoAxis>(&entity->geom);
        if (!gizmo) continue;
        
        // 计算射线与轴的最近距离
        glm::vec3 axisEnd = gizmo->origin + gizmo->direction * gizmo->length;
        
        // 使用射线与线段的距离计算
        glm::vec3 w0 = ray.getOrigin() - gizmo->origin;
        glm::vec3 u = ray.getDirection();
        glm::vec3 v = gizmo->direction;
        
        float a = glm::dot(u, u);  // 总是 1（归一化）
        float b = glm::dot(u, v);
        float c = glm::dot(v, v);  // 总是 1（归一化）
        float d = glm::dot(u, w0);
        float e = glm::dot(v, w0);
        
        float denom = a * c - b * b;
        if (std::abs(denom) < 1e-6f) {
            // 平行或重合
            continue;
        }
        
        float sc = (b * e - c * d) / denom;
        float tc = (a * e - b * d) / denom;
        
        // 检查 tc 是否在轴的有效范围内
        if (tc < 0.0f || tc > gizmo->length) {
            continue;
        }
        
        // 计算最近点的距离
        glm::vec3 closestOnRay = ray.getOrigin() + u * sc;
        glm::vec3 closestOnAxis = gizmo->origin + v * tc;
        float dist = glm::distance(closestOnRay, closestOnAxis);
        
        // 检查是否在阈值内，并且是最近的
        if (dist < threshold && dist < minDistance) {
            minDistance = dist;
            pickedAxis = gizmo->axisIndex;
        }
    }
    
    return pickedAxis;
}

// ============================================
// 2D 距离计算实现
// ============================================

float Picker::distanceToLine2D(
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

float Picker::distanceToPolyline2D(
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
    
    // ✅ Lambda：检查一条线段并更新最小距离
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
    
    // 检查所有线段
    for (size_t i = 0; i < polyline.pts.size() - 1; ++i) {
        checkSegment(polyline.pts[i], polyline.pts[i + 1]);
    }
    
    // 如果是闭合多段线，检查首尾连接线段
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

float Picker::distanceToCircle2D(
    const glm::vec3& point,
    const Circle& circle,
    const ViewportState& vp,
    glm::vec3* closestPoint) const {
    
    // 屏幕空间计算
    glm::vec2 p = vp.worldToScreen(point);
    glm::vec2 center = vp.worldToScreen(circle.c);
    
    // ⭐ 修正：在圆心位置计算实际的像素大小
    // 计算圆周上一个点（沿X轴偏移半径）
    glm::vec3 circleEdgePoint = circle.c + glm::vec3(circle.r, 0.0f, 0.0f);
    glm::vec2 edgeScreen = vp.worldToScreen(circleEdgePoint);
    
    // 圆在屏幕上的实际半径（像素）
    float radiusPixel = glm::distance(center, edgeScreen);
    
    // 点到圆心的距离（屏幕像素）
    float distToCenter = glm::distance(p, center);
    
    // 点到圆周的距离（屏幕像素）
    float distToCirclePixel = std::abs(distToCenter - radiusPixel);
    
    if (closestPoint) {
        // 计算圆周上最近的点（屏幕空间）
        if (distToCenter > 1e-6f) {
            glm::vec2 direction = glm::normalize(p - center);
            glm::vec2 closest2D = center + direction * radiusPixel;
            
            *closestPoint = vp.screenToWorld(
                static_cast<int>(closest2D.x),
                static_cast<int>(closest2D.y),
                circle.c.z
            );
        } else {
            // 点在圆心，任意选择一个圆周点
            *closestPoint = circleEdgePoint;
        }
    }
    
    // ⭐ 返回世界空间距离
    // 方法1: 使用圆心处的像素大小转换
    float pixelSizeAtCenter = vp.getPixelSizeAt(circle.c);
    return distToCirclePixel * pixelSizeAtCenter;
    
    // 方法2（更精确）: 直接计算世界空间距离
    // if (closestPoint) {
    //     return glm::distance(point, *closestPoint);
    // } else {
    //     glm::vec3 tmpClosest;
    //     glm::vec2 direction = glm::normalize(p - center);
    //     glm::vec2 closest2D = center + direction * radiusPixel;
    //     tmpClosest = vp.screenToWorld(
    //         static_cast<int>(closest2D.x),
    //         static_cast<int>(closest2D.y),
    //         circle.c.z
    //     );
    //     return glm::distance(point, tmpClosest);
    // }
}

float Picker::distanceToArc2D(
    const glm::vec3& point,
    const Arc& arc,
    const ViewportState& vp,
    glm::vec3* closestPoint) const {
    
    // ⭐ 直接在世界空间计算（2D，忽略Z）
    glm::vec2 p2D(point.x, point.y);
    glm::vec2 center2D(arc.c.x, arc.c.y);
    
    // 计算点相对于圆心的角度（世界空间）
    glm::vec2 toPoint = p2D - center2D;
    float angle = std::atan2(toPoint.y, toPoint.x);
    
    // 归一化到 [0, 2π]
    if (angle < 0) angle += 2.0f * glm::pi<float>();
    
    // 检查角度是否在圆弧范围内
    float a0 = arc.a0;
    float a1 = arc.a1;
    
    // 处理跨越 0 度的情况
    bool inArc = false;
    if (a0 <= a1) {
        inArc = (angle >= a0 && angle <= a1);
    } else {
        inArc = (angle >= a0 || angle <= a1);
    }
    
    if (inArc) {
        // 在圆弧范围内，计算到圆弧的距离
        float distToCenter = glm::distance(p2D, center2D);
        float distToArc = std::abs(distToCenter - arc.r);
        
        if (closestPoint) {
            if (glm::length(toPoint) > 1e-6f) {
                glm::vec2 direction = glm::normalize(toPoint);
                glm::vec2 closest2D = center2D + direction * arc.r;
                *closestPoint = glm::vec3(closest2D.x, closest2D.y, arc.c.z);
            } else {
                // 点在圆心
                *closestPoint = glm::vec3(
                    arc.c.x + arc.r, 
                    arc.c.y, 
                    arc.c.z
                );
            }
        }
        
        return distToArc;
        
    } else {
        // 不在圆弧范围内，计算到两个端点的距离
        
        // 计算两个端点的世界坐标
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

float Picker::distanceToBox2D(
    const glm::vec3& point,
    const Box& box,
    const ViewportState& vp,
    glm::vec3* closestPoint) const {
    
    // 立方体的 8 个顶点
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
    
    // 立方体的 12 条边
    int edges[12][2] = {
        {0, 1}, {1, 2}, {2, 3}, {3, 0},  // 后面
        {4, 5}, {5, 6}, {6, 7}, {7, 4},  // 前面
        {0, 4}, {1, 5}, {2, 6}, {3, 7}   // 连接线
    };
    
    glm::vec2 p = vp.worldToScreen(point);
    
    float minDist = std::numeric_limits<float>::max();
    glm::vec2 closestPt2D;
    
    // 检查所有边
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

// ============================================
// 几何辅助方法
// ============================================

float Picker::pointToSegmentDistance(
    const glm::vec2& p,
    const glm::vec2& a,
    const glm::vec2& b,
    glm::vec2* closestPoint) {
    
    glm::vec2 ab = b - a;
    glm::vec2 ap = p - a;
    
    float abLenSq = glm::dot(ab, ab);
    
    if (abLenSq < 1e-6f) {
        // 线段退化为点
        if (closestPoint) {
            *closestPoint = a;
        }
        return glm::distance(p, a);
    }
    
    // 参数 t：p 在 ab 上的投影位置
    float t = glm::dot(ap, ab) / abLenSq;
    t = glm::clamp(t, 0.0f, 1.0f);
    
    // 最近点
    glm::vec2 closest = a + t * ab;
    
    if (closestPoint) {
        *closestPoint = closest;
    }
    
    return glm::distance(p, closest);
}

// ============================================
// 2D 框选功能实现
// ============================================

std::vector<EntityId> Picker::selectByBox2D(
    EntityId boxEntityId,
    Document& document,
    BoxSelectMode mode) const {
    
    const Entity* boxEntity = document.get(boxEntityId);
    if (!boxEntity || boxEntity->type != EntityType::Rectangle) {
        return {};
    }
    
    const Rectangle* rect = std::get_if<Rectangle>(&boxEntity->geom);
    if (!rect) {
        return {};
    }
    
    float minX = std::min(rect->p0.x, rect->p1.x);
    float maxX = std::max(rect->p0.x, rect->p1.x);
    float minY = std::min(rect->p0.y, rect->p1.y);
    float maxY = std::max(rect->p0.y, rect->p1.y);
    
    std::vector<EntityId> selectedIds;
    
    // 3. 遍历所有实体，检查是否与矩形框相交或被包含
    for (auto* entity : document.all()) {
        if (!entity || entity->id == boxEntityId) {
            continue;  // 跳过矩形框自己
        }
        
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
                if (auto* otherRect = std::get_if<Rectangle>(&entity->geom)) {
                    shouldSelect = checkRectangleInBox2D(*otherRect, minX, minY, maxX, maxY, mode);
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
        
        // 4. 设置 hover 状态
        if (shouldSelect) {
            if (!entity->hovered) {
                entity->hovered = true;
                entity->dirty = true;  // 标记需要更新渲染状态
            }
            selectedIds.push_back(entity->id);
        } else {
            if (entity->hovered) {
                entity->hovered = false;
                entity->dirty = true;  // ⭐ 关键：取消 hover 时也要标记为 dirty
            }
        }
    }
    
    return selectedIds;
}

// ============================================
// 2D 框选几何检测实现
// ============================================

bool Picker::isPoint2DInBox(
    float px, float py,
    float minX, float minY, float maxX, float maxY) const {
    return px >= minX && px <= maxX && py >= minY && py <= maxY;
}

bool Picker::checkLineInBox2D(
    const Line& line,
    float minX, float minY, float maxX, float maxY,
    BoxSelectMode mode) const {
    
    bool p0In = isPoint2DInBox(line.p0.x, line.p0.y, minX, minY, maxX, maxY);
    bool p1In = isPoint2DInBox(line.p1.x, line.p1.y, minX, minY, maxX, maxY);
    
    if (mode == BoxSelectMode::CONTAIN) {
        // 完全包含：两个端点都在框内
        return p0In && p1In;
    } else {
        // 相交：任一端点在框内，或线段与框边界相交
        return p0In || p1In || 
               lineSegmentIntersectsBox2D(line.p0.x, line.p0.y, line.p1.x, line.p1.y,
                                         minX, minY, maxX, maxY);
    }
}

bool Picker::checkPolylineInBox2D(
    const Polyline& polyline,
    float minX, float minY, float maxX, float maxY,
    BoxSelectMode mode) const {
    
    if (polyline.pts.empty()) {
        return false;
    }
    
    if (mode == BoxSelectMode::CONTAIN) {
        // 完全包含：所有点都在框内
        for (const auto& pt : polyline.pts) {
            if (!isPoint2DInBox(pt.x, pt.y, minX, minY, maxX, maxY)) {
                return false;
            }
        }
        return true;
        
    } else {
        // 相交：任意点在框内，或任意线段与框相交
        for (const auto& pt : polyline.pts) {
            if (isPoint2DInBox(pt.x, pt.y, minX, minY, maxX, maxY)) {
                return true;
            }
        }
        
        // 检查线段相交
        for (size_t i = 0; i < polyline.pts.size() - 1; ++i) {
            const auto& p0 = polyline.pts[i];
            const auto& p1 = polyline.pts[i + 1];
            if (lineSegmentIntersectsBox2D(p0.x, p0.y, p1.x, p1.y,
                                          minX, minY, maxX, maxY)) {
                return true;
            }
        }
        
        // 闭合多段线需要检查首尾连接
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

bool Picker::checkRectangleInBox2D(
    const Rectangle& rect,
    float minX, float minY, float maxX, float maxY,
    BoxSelectMode mode) const {
    
    float rectMinX = std::min(rect.p0.x, rect.p1.x);
    float rectMaxX = std::max(rect.p0.x, rect.p1.x);
    float rectMinY = std::min(rect.p0.y, rect.p1.y);
    float rectMaxY = std::max(rect.p0.y, rect.p1.y);
    
    if (mode == BoxSelectMode::CONTAIN) {
        // 完全包含：矩形的所有角都在框内
        return rectMinX >= minX && rectMaxX <= maxX &&
               rectMinY >= minY && rectMaxY <= maxY;
    } else {
        // 相交：两个矩形的 AABB 相交
        return !(rectMaxX < minX || rectMinX > maxX ||
                 rectMaxY < minY || rectMinY > maxY);
    }
}

bool Picker::checkCircleInBox2D(
    const Circle& circle,
    float minX, float minY, float maxX, float maxY,
    BoxSelectMode mode) const {
    
    float cx = circle.c.x;
    float cy = circle.c.y;
    float r = circle.r;
    
    if (mode == BoxSelectMode::CONTAIN) {
        // 完全包含：圆完全在框内（包括边界）
        return (cx - r >= minX) && (cx + r <= maxX) &&
               (cy - r >= minY) && (cy + r <= maxY);
    } else {
        // 相交：圆与矩形相交
        // 找到矩形上距离圆心最近的点
        float closestX = std::clamp(cx, minX, maxX);
        float closestY = std::clamp(cy, minY, maxY);
        
        // 计算距离
        float dx = cx - closestX;
        float dy = cy - closestY;
        float distSq = dx * dx + dy * dy;
        
        // 如果距离小于半径，则相交
        return distSq <= (r * r);
    }
}

bool Picker::checkArcInBox2D(
    const Arc& arc,
    float minX, float minY, float maxX, float maxY,
    BoxSelectMode mode) const {
    
    // 简化实现：检查圆弧的端点和采样点
    float cx = arc.c.x;
    float cy = arc.c.y;
    float r = arc.r;
    
    // 计算圆弧的两个端点
    float x0 = cx + r * std::cos(arc.a0);
    float y0 = cy + r * std::sin(arc.a0);
    float x1 = cx + r * std::cos(arc.a1);
    float y1 = cy + r * std::sin(arc.a1);
    
    bool p0In = isPoint2DInBox(x0, y0, minX, minY, maxX, maxY);
    bool p1In = isPoint2DInBox(x1, y1, minX, minY, maxX, maxY);
    
    if (mode == BoxSelectMode::CONTAIN) {
        // 完全包含：采样多个点检查是否都在框内
        const int samples = 16;
        float angleRange = arc.a1 - arc.a0;
        if (angleRange < 0) angleRange += 2.0f * glm::pi<float>();
        
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
        // 相交：端点在框内，或圆弧与框边界相交
        if (p0In || p1In) {
            return true;
        }
        
        // 简化检查：采样多个点
        const int samples = 16;
        float angleRange = arc.a1 - arc.a0;
        if (angleRange < 0) angleRange += 2.0f * glm::pi<float>();
        
        for (int i = 0; i <= samples; ++i) {
            float t = float(i) / samples;
            float angle = arc.a0 + t * angleRange;
            float px = cx + r * std::cos(angle);
            float py = cy + r * std::sin(angle);
            
            if (isPoint2DInBox(px, py, minX, minY, maxX, maxY)) {
                return true;
            }
        }
        
        // 还需要检查圆弧是否穿过矩形（矩形完全在圆弧内部的情况）
        // 简化：检查矩形的四个角到圆心的距离
        bool boxCenterIn = isPoint2DInBox(cx, cy, minX, minY, maxX, maxY);
        if (boxCenterIn && r > 0) {
            // 圆心在框内，可能相交
            return true;
        }
        
        return false;
    }
}

bool Picker::checkBox3DInBox2D(
    const Box& box,
    float minX, float minY, float maxX, float maxY,
    BoxSelectMode mode) const {
    
    // 3D 立方体投影到 2D：检查中心点和 8 个顶点的 XY 坐标
    float half = box.size * 0.5f;
    glm::vec3 c = box.center;
    
    // 8 个顶点
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
        // 完全包含：所有顶点的 XY 坐标都在框内
        for (int i = 0; i < 8; ++i) {
            if (!isPoint2DInBox(vertices[i].x, vertices[i].y, minX, minY, maxX, maxY)) {
                return false;
            }
        }
        return true;
        
    } else {
        // 相交：任意顶点在框内
        for (int i = 0; i < 8; ++i) {
            if (isPoint2DInBox(vertices[i].x, vertices[i].y, minX, minY, maxX, maxY)) {
                return true;
            }
        }
        
        // 或者立方体的边与框相交（简化：检查投影后的 AABB）
        float boxMinX = vertices[0].x, boxMaxX = vertices[0].x;
        float boxMinY = vertices[0].y, boxMaxY = vertices[0].y;
        
        for (int i = 1; i < 8; ++i) {
            boxMinX = std::min(boxMinX, vertices[i].x);
            boxMaxX = std::max(boxMaxX, vertices[i].x);
            boxMinY = std::min(boxMinY, vertices[i].y);
            boxMaxY = std::max(boxMaxY, vertices[i].y);
        }
        
        // AABB 相交检测
        return !(boxMaxX < minX || boxMinX > maxX ||
                 boxMaxY < minY || boxMinY > maxY);
    }
}

bool Picker::lineSegmentIntersectsBox2D(
    float x0, float y0, float x1, float y1,
    float minX, float minY, float maxX, float maxY) const {
    
    // Cohen-Sutherland 线段裁剪算法
    // 编码：左(0001), 右(0010), 下(0100), 上(1000)
    
    auto computeCode = [&](float x, float y) -> int {
        int code = 0;
        if (x < minX) code |= 1;      // 左
        if (x > maxX) code |= 2;      // 右
        if (y < minY) code |= 4;      // 下
        if (y > maxY) code |= 8;      // 上
        return code;
    };
    
    int code0 = computeCode(x0, y0);
    int code1 = computeCode(x1, y1);
    
    // 简化判断
    if (code0 == 0 || code1 == 0) {
        // 至少一个端点在框内
        return true;
    }
    
    if ((code0 & code1) != 0) {
        // 两个端点在框的同一侧外面
        return false;
    }
    
    // 可能相交，需要详细计算（这里简化为返回 true）
    // 完整实现应该进行线段裁剪计算
    return true;
}

// ============================================
// 3D 框选功能实现（基于工作平面和射线）
// ============================================

std::vector<EntityId> Picker::selectByBox3D(
    EntityId boxEntityId,
    Document& document,
    const ViewportState& vp,
    const WorkPlane& workPlane,
    BoxSelectMode mode) const {
    
    const Entity* boxEntity = document.get(boxEntityId);
    if (!boxEntity || boxEntity->type != EntityType::Rectangle) {
        return {};
    }
    
    const Rectangle* rect = std::get_if<Rectangle>(&boxEntity->geom);
    if (!rect) {
        return {};
    }
    
    glm::vec3 boxCorners[4] = {
        rect->p0,
        glm::vec3(rect->p1.x, rect->p0.y, rect->p0.z),
        rect->p1,
        glm::vec3(rect->p0.x, rect->p1.y, rect->p0.z)
    };
    
    std::vector<EntityId> selectedIds;
    
    for (auto* entity : document.all()) {
        if (!entity || entity->id == boxEntityId) {
            continue;
        }
        
        bool shouldSelect = checkEntityInBox3D(*entity, boxCorners, vp, mode);
        
        if (shouldSelect) {
            if (!entity->hovered) {
                entity->hovered = true;
                entity->dirty = true;
            }
            selectedIds.push_back(entity->id);
        } else {
            if (entity->hovered) {
                entity->hovered = false;
                entity->dirty = true;
            }
        }
    }
    
    return selectedIds;
}

// ============================================
// 3D 框选几何检测实现
// ============================================

bool Picker::checkEntityInBox3D(
    const Entity& entity,
    const glm::vec3 boxCorners[4],
    const ViewportState& vp,
    BoxSelectMode mode) const {
    
    // 策略：
    // 1. INTERSECT 模式：从框的边界和内部发射射线网格，只要有任意射线击中实体即选中
    // 2. CONTAIN 模式：检查实体的关键点（顶点、中心等）是否都在框内
    
    if (mode == BoxSelectMode::CONTAIN) {
        // 完全包含模式：检查实体的所有关键点是否都在框的投影范围内
        // 这里简化实现：只检查端点/中心点
        
        std::vector<glm::vec3> keyPoints;
        
        switch (entity.type) {
            case EntityType::Line: {
                if (auto* line = std::get_if<Line>(&entity.geom)) {
                    keyPoints = {line->p0, line->p1};
                }
                break;
            }
            
            case EntityType::Polyline: {
                if (auto* polyline = std::get_if<Polyline>(&entity.geom)) {
                    keyPoints = polyline->pts;
                }
                break;
            }
            
            case EntityType::Rectangle: {
                if (auto* r = std::get_if<Rectangle>(&entity.geom)) {
                    keyPoints = {
                        r->p0,
                        glm::vec3(r->p1.x, r->p0.y, r->p0.z),
                        r->p1,
                        glm::vec3(r->p0.x, r->p1.y, r->p0.z)
                    };
                }
                break;
            }
            
            case EntityType::Circle: {
                if (auto* circle = std::get_if<Circle>(&entity.geom)) {
                    // 简化：只检查圆心和四个基本方向的点
                    keyPoints = {
                        circle->c,
                        circle->c + glm::vec3(circle->r, 0, 0),
                        circle->c + glm::vec3(-circle->r, 0, 0),
                        circle->c + glm::vec3(0, circle->r, 0),
                        circle->c + glm::vec3(0, -circle->r, 0)
                    };
                }
                break;
            }
            
            case EntityType::Arc: {
                if (auto* arc = std::get_if<Arc>(&entity.geom)) {
                    keyPoints = {arc->c};
                }
                break;
            }
            
            case EntityType::Box: {
                if (auto* box = std::get_if<Box>(&entity.geom)) {
                    float half = box->size * 0.5f;
                    keyPoints = {
                        box->center + glm::vec3(-half, -half, -half),
                        box->center + glm::vec3( half, -half, -half),
                        box->center + glm::vec3( half,  half, -half),
                        box->center + glm::vec3(-half,  half, -half),
                        box->center + glm::vec3(-half, -half,  half),
                        box->center + glm::vec3( half, -half,  half),
                        box->center + glm::vec3( half,  half,  half),
                        box->center + glm::vec3(-half,  half,  half),
                    };
                }
                break;
            }
        }
        
        // 检查所有关键点是否都在屏幕空间的框内
        if (keyPoints.empty()) return false;
        
        // 计算框在屏幕空间的边界
        glm::vec2 screenCorners[4];
        for (int i = 0; i < 4; ++i) {
            screenCorners[i] = vp.worldToScreen(boxCorners[i]);
        }
        
        float minX = std::min({screenCorners[0].x, screenCorners[1].x, screenCorners[2].x, screenCorners[3].x});
        float maxX = std::max({screenCorners[0].x, screenCorners[1].x, screenCorners[2].x, screenCorners[3].x});
        float minY = std::min({screenCorners[0].y, screenCorners[1].y, screenCorners[2].y, screenCorners[3].y});
        float maxY = std::max({screenCorners[0].y, screenCorners[1].y, screenCorners[2].y, screenCorners[3].y});
        
        for (const auto& pt : keyPoints) {
            glm::vec2 screenPt = vp.worldToScreen(pt);
            if (screenPt.x < minX || screenPt.x > maxX || 
                screenPt.y < minY || screenPt.y > maxY) {
                return false;  // 有点在框外，不满足完全包含
            }
        }
        
        return true;  // 所有点都在框内
        
    } else {
        std::vector<Ray> rays = generateBoxRays(boxCorners, vp, 8);
        
        for (const auto& ray : rays) {
            glm::vec3 hitPoint;
            bool hit = false;
            
            switch (entity.type) {
                case EntityType::Line: {
                    if (auto* line = std::get_if<Line>(&entity.geom)) {
                        hit = intersectLine(ray, *line, hitPoint, 0.1f);
                    }
                    break;
                }
                
                case EntityType::Polyline: {
                    if (auto* polyline = std::get_if<Polyline>(&entity.geom)) {
                        hit = intersectPolyline(ray, *polyline, hitPoint, 0.1f);
                    }
                    break;
                }
                
                case EntityType::Circle: {
                    if (auto* circle = std::get_if<Circle>(&entity.geom)) {
                        hit = intersectCircle(ray, *circle, hitPoint, 0.1f);
                    }
                    break;
                }
                
                case EntityType::Arc: {
                    if (auto* arc = std::get_if<Arc>(&entity.geom)) {
                        hit = intersectArc(ray, *arc, hitPoint, 0.1f);
                    }
                    break;
                }
                
                case EntityType::Box: {
                    if (auto* box = std::get_if<Box>(&entity.geom)) {
                        hit = intersectBox(ray, *box, hitPoint);
                    }
                    break;
                }
                
                case EntityType::Rectangle: {
                    // 矩形暂时当作4条线段处理
                    if (auto* r = std::get_if<Rectangle>(&entity.geom)) {
                        Line edges[4] = {
                            {r->p0, glm::vec3(r->p1.x, r->p0.y, r->p0.z)},
                            {glm::vec3(r->p1.x, r->p0.y, r->p0.z), r->p1},
                            {r->p1, glm::vec3(r->p0.x, r->p1.y, r->p0.z)},
                            {glm::vec3(r->p0.x, r->p1.y, r->p0.z), r->p0}
                        };
                        for (int i = 0; i < 4; ++i) {
                            if (intersectLine(ray, edges[i], hitPoint, 0.1f)) {
                                hit = true;
                                break;
                            }
                        }
                    }
                    break;
                }
            }
            
            if (hit) {
                return true;
            }
        }
        
        return false;
    }
}

std::vector<Ray> Picker::generateBoxRays(
    const glm::vec3 boxCorners[4],
    const ViewportState& vp,
    int sampleCount) const {
    
    std::vector<Ray> rays;
    
    // 1. 从四个角点发射射线
    for (int i = 0; i < 4; ++i) {
        glm::vec2 screenPos = vp.worldToScreen(boxCorners[i]);
        Ray ray = Ray::fromScreen(
            static_cast<int>(screenPos.x),
            static_cast<int>(screenPos.y),
            vp.width, vp.height,
            vp.view, vp.proj
        );
        rays.push_back(ray);
    }
    
    // 2. 从框的四条边上采样点发射射线
    for (int edge = 0; edge < 4; ++edge) {
        int nextEdge = (edge + 1) % 4;
        
        for (int sample = 1; sample < sampleCount; ++sample) {
            float t = float(sample) / float(sampleCount);
            glm::vec3 samplePoint = glm::mix(boxCorners[edge], boxCorners[nextEdge], t);
            
            glm::vec2 screenPos = vp.worldToScreen(samplePoint);
            Ray ray = Ray::fromScreen(
                static_cast<int>(screenPos.x),
                static_cast<int>(screenPos.y),
                vp.width, vp.height,
                vp.view, vp.proj
            );
            rays.push_back(ray);
        }
    }
    
    // 3. 从框内部采样点发射射线（网格）
    for (int i = 1; i < sampleCount; ++i) {
        for (int j = 1; j < sampleCount; ++j) {
            float u = float(i) / float(sampleCount);
            float v = float(j) / float(sampleCount);
            
            // 双线性插值计算内部点
            glm::vec3 bottom = glm::mix(boxCorners[0], boxCorners[1], u);
            glm::vec3 top = glm::mix(boxCorners[3], boxCorners[2], u);
            glm::vec3 samplePoint = glm::mix(bottom, top, v);
            
            glm::vec2 screenPos = vp.worldToScreen(samplePoint);
            Ray ray = Ray::fromScreen(
                static_cast<int>(screenPos.x),
                static_cast<int>(screenPos.y),
                vp.width, vp.height,
                vp.view, vp.proj
            );
            rays.push_back(ray);
        }
    }
    
    return rays;
}

bool Picker::isPointInBox3D(
    const glm::vec3& point,
    const glm::vec3 boxCorners[4],
    const WorkPlane& workPlane) const {
    
    // 将点和框都转换到工作平面的局部坐标系
    glm::vec2 localPoint = workPlane.worldToLocal(point);
    
    glm::vec2 localCorners[4];
    for (int i = 0; i < 4; ++i) {
        localCorners[i] = workPlane.worldToLocal(boxCorners[i]);
    }
    
    // 计算框的边界
    float minX = std::min({localCorners[0].x, localCorners[1].x, localCorners[2].x, localCorners[3].x});
    float maxX = std::max({localCorners[0].x, localCorners[1].x, localCorners[2].x, localCorners[3].x});
    float minY = std::min({localCorners[0].y, localCorners[1].y, localCorners[2].y, localCorners[3].y});
    float maxY = std::max({localCorners[0].y, localCorners[1].y, localCorners[2].y, localCorners[3].y});
    
    // 检查点是否在边界内
    return localPoint.x >= minX && localPoint.x <= maxX &&
           localPoint.y >= minY && localPoint.y <= maxY;
}
