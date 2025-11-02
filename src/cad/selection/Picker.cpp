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
            
            case EntityType::Box: {
                if (auto* box = std::get_if<Box>(&entity->geom)) {
                    distance = distanceToBox2D(worldPos, *box, vp, &closestPoint);
                }
                break;
            }
        }
        
        // 检查是否在阈值内
        if (distance <= worldThreshold) {
            results.push_back({entity->id, closestPoint, distance});
        }
    }
    
    qDebug() << "res size : " << results.size();
    // 按距离排序
    std::sort(results.begin(), results.end());
    
    return results;
}

// ============================================
// 2D 距离计算实现
// ============================================

float Picker::distanceToLine2D(
    const glm::vec3& point,
    const Line& line,
    const ViewportState& vp,
    glm::vec3* closestPoint) const {
    
    // 将 3D 点投影到屏幕坐标
    glm::vec2 p = vp.worldToScreen(point);
    glm::vec2 a = vp.worldToScreen(line.p0);
    glm::vec2 b = vp.worldToScreen(line.p1);
    
    glm::vec2 closest2D;
    float pixelDist = pointToSegmentDistance(p, a, b, &closest2D);

    qDebug() << "======start======";
    qDebug() << p.x << " - " << p.y;
    qDebug() << a.x << " - " << a.y;
    qDebug() << b.x << " - " << b.y;
    qDebug() << closest2D.x << " - " << closest2D.y;
    qDebug() << pixelDist << " - " << pixelDist * vp.worldPerPixel;
    qDebug() << "======end======";
    
    if (closestPoint) {
        // 将屏幕坐标转回世界坐标
        *closestPoint = vp.screenToWorld(
            static_cast<int>(closest2D.x),
            static_cast<int>(closest2D.y),
            point.z  // 保持相同的 Z 平面
        );
    }
    
    // 转换为世界单位距离
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
    
    // 圆在屏幕上的半径（像素）
    float radiusPixel = circle.r / vp.worldPerPixel;
    
    // 点到圆心的距离
    float distToCenter = glm::distance(p, center);
    
    // 点到圆周的距离
    float distToCircle = std::abs(distToCenter - radiusPixel);

    qDebug() << "cicle calc : " << radiusPixel << " - " << distToCenter;
    qDebug() << "cicle dis : " << glm::distance(glm::vec2(point.x, point.y), glm::vec2(circle.c.x, circle.c.y));
    
    if (closestPoint) {
        // 计算圆周上最近的点
        glm::vec2 direction = glm::normalize(p - center);
        glm::vec2 closest2D = center + direction * radiusPixel;
        
        *closestPoint = vp.screenToWorld(
            static_cast<int>(closest2D.x),
            static_cast<int>(closest2D.y),
            circle.c.z
        );
    }
    
    return distToCircle * vp.worldPerPixel;
}

float Picker::distanceToArc2D(
    const glm::vec3& point,
    const Arc& arc,
    const ViewportState& vp,
    glm::vec3* closestPoint) const {
    
    // 屏幕空间计算
    glm::vec2 p = vp.worldToScreen(point);
    glm::vec2 center = vp.worldToScreen(arc.c);
    
    float radiusPixel = arc.r / vp.worldPerPixel;
    
    // 计算点相对于圆心的角度
    glm::vec2 toPoint = p - center;
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
        float distToCenter = glm::distance(p, center);
        float distToArc = std::abs(distToCenter - radiusPixel);
        
        if (closestPoint) {
            glm::vec2 direction = glm::normalize(toPoint);
            glm::vec2 closest2D = center + direction * radiusPixel;
            *closestPoint = vp.screenToWorld(
                static_cast<int>(closest2D.x),
                static_cast<int>(closest2D.y),
                arc.c.z
            );
        }
        
        return distToArc * vp.worldPerPixel;
    } else {
        // 不在圆弧范围内，计算到两个端点的距离
        glm::vec2 p0(center.x + radiusPixel * std::cos(a0),
                     center.y + radiusPixel * std::sin(a0));
        glm::vec2 p1(center.x + radiusPixel * std::cos(a1),
                     center.y + radiusPixel * std::sin(a1));
        
        float dist0 = glm::distance(p, p0);
        float dist1 = glm::distance(p, p1);
        
        if (dist0 < dist1) {
            if (closestPoint) {
                *closestPoint = vp.screenToWorld(
                    static_cast<int>(p0.x),
                    static_cast<int>(p0.y),
                    arc.c.z
                );
            }
            return dist0 * vp.worldPerPixel;
        } else {
            if (closestPoint) {
                *closestPoint = vp.screenToWorld(
                    static_cast<int>(p1.x),
                    static_cast<int>(p1.y),
                    arc.c.z
                );
            }
            return dist1 * vp.worldPerPixel;
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
