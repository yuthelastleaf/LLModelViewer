#include "Transform.h"
#include <algorithm>

std::optional<glm::vec3> Transform::getEntityCenter(const Entity* entity) {
    if (!entity) return std::nullopt;
    
    switch (entity->type) {
        case EntityType::Line: {
            const auto& line = std::get<Line>(entity->geom);
            // 线段中点
            return (line.p0 + line.p1) * 0.5f;
        }
        
        case EntityType::Polyline: {
            const auto& polyline = std::get<Polyline>(entity->geom);
            if (polyline.pts.empty()) return std::nullopt;
            // 所有顶点的平均值
            return computeAverage(polyline.pts);
        }
        
        case EntityType::Rectangle: {
            const auto& rect = std::get<Rectangle>(entity->geom);
            // 对角线中点
            return (rect.p0 + rect.p1) * 0.5f;
        }
        
        case EntityType::Circle: {
            const auto& circle = std::get<Circle>(entity->geom);
            // 圆心
            return circle.c;
        }
        
        case EntityType::Arc: {
            const auto& arc = std::get<Arc>(entity->geom);
            // 圆弧圆心（注意：不是几何中心，但对于变换来说圆心更合适）
            return arc.c;
        }
        
        case EntityType::Box: {
            const auto& box = std::get<Box>(entity->geom);
            // 立方体中心
            return box.center;
        }
        
        default:
            return std::nullopt;
    }
}

glm::vec3 Transform::getSelectionCenter(const std::vector<Entity*>& entities) {
    if (entities.empty()) {
        return glm::vec3(0.0f);
    }
    
    std::vector<glm::vec3> centers;
    centers.reserve(entities.size());
    
    for (const auto* entity : entities) {
        auto center = getEntityCenter(entity);
        if (center.has_value()) {
            centers.push_back(center.value());
        }
    }
    
    return computeAverage(centers);
}

void Transform::translateEntity(Entity* entity, const glm::vec3& offset) {
    if (!entity) return;
    
    switch (entity->type) {
        case EntityType::Line: {
            auto& line = std::get<Line>(entity->geom);
            line.p0 += offset;
            line.p1 += offset;
            break;
        }
        
        case EntityType::Polyline: {
            auto& polyline = std::get<Polyline>(entity->geom);
            for (auto& pt : polyline.pts) {
                pt += offset;
            }
            break;
        }
        
        case EntityType::Rectangle: {
            auto& rect = std::get<Rectangle>(entity->geom);
            rect.p0 += offset;
            rect.p1 += offset;
            break;
        }
        
        case EntityType::Circle: {
            auto& circle = std::get<Circle>(entity->geom);
            circle.c += offset;
            break;
        }
        
        case EntityType::Arc: {
            auto& arc = std::get<Arc>(entity->geom);
            arc.c += offset;
            break;
        }
        
        case EntityType::Box: {
            auto& box = std::get<Box>(entity->geom);
            box.center += offset;
            break;
        }
    }
    
    entity->dirty = true;
}

void Transform::translateEntities(const std::vector<Entity*>& entities, const glm::vec3& offset) {
    for (auto* entity : entities) {
        translateEntity(entity, offset);
    }
}

bool Transform::getEntityBounds(const Entity* entity, glm::vec3& outMin, glm::vec3& outMax) {
    if (!entity) return false;
    
    std::vector<glm::vec3> points;
    
    switch (entity->type) {
        case EntityType::Line: {
            const auto& line = std::get<Line>(entity->geom);
            points = {line.p0, line.p1};
            break;
        }
        
        case EntityType::Polyline: {
            const auto& polyline = std::get<Polyline>(entity->geom);
            points = polyline.pts;
            break;
        }
        
        case EntityType::Rectangle: {
            const auto& rect = std::get<Rectangle>(entity->geom);
            points = {rect.p0, rect.p1};
            break;
        }
        
        case EntityType::Circle: {
            const auto& circle = std::get<Circle>(entity->geom);
            // 圆的包围盒
            glm::vec3 r(circle.r, circle.r, 0.0f);
            points = {circle.c - r, circle.c + r};
            break;
        }
        
        case EntityType::Arc: {
            const auto& arc = std::get<Arc>(entity->geom);
            // 简化：使用圆的包围盒
            glm::vec3 r(arc.r, arc.r, 0.0f);
            points = {arc.c - r, arc.c + r};
            break;
        }
        
        case EntityType::Box: {
            const auto& box = std::get<Box>(entity->geom);
            float half = box.size * 0.5f;
            glm::vec3 h(half, half, half);
            points = {box.center - h, box.center + h};
            break;
        }
        
        default:
            return false;
    }
    
    if (points.empty()) return false;
    
    // 计算包围盒
    outMin = points[0];
    outMax = points[0];
    
    for (const auto& pt : points) {
        outMin.x = std::min(outMin.x, pt.x);
        outMin.y = std::min(outMin.y, pt.y);
        outMin.z = std::min(outMin.z, pt.z);
        
        outMax.x = std::max(outMax.x, pt.x);
        outMax.y = std::max(outMax.y, pt.y);
        outMax.z = std::max(outMax.z, pt.z);
    }
    
    return true;
}

glm::vec3 Transform::computeAverage(const std::vector<glm::vec3>& points) {
    if (points.empty()) {
        return glm::vec3(0.0f);
    }
    
    glm::vec3 sum(0.0f);
    for (const auto& pt : points) {
        sum += pt;
    }
    
    return sum / static_cast<float>(points.size());
}
