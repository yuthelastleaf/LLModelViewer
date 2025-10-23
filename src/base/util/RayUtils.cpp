#include "RayUtils.h"
#include <cmath>
#include <algorithm>

// ============================================
// 构造函数
// ============================================

Ray::Ray()
    : origin_(0.0f, 0.0f, 0.0f)
    , direction_(0.0f, 0.0f, -1.0f)
{
}

Ray::Ray(const glm::vec3& origin, const glm::vec3& direction)
    : origin_(origin)
    , direction_(glm::normalize(direction))
{
}

// ============================================
// 从屏幕坐标生成射线
// ============================================

Ray Ray::fromScreen(
    int screenX, int screenY,
    int viewportWidth, int viewportHeight,
    const glm::mat4& viewMatrix,
    const glm::mat4& projMatrix) {
    
    // 步骤 1：屏幕坐标 → NDC（归一化设备坐标）
    // 屏幕坐标：左上角 (0, 0)，右下角 (width, height)
    // NDC 坐标：左下角 (-1, -1)，右上角 (1, 1)
    float ndcX = (2.0f * screenX) / viewportWidth - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY) / viewportHeight;  // Y 轴翻转
    
    return fromNDC(ndcX, ndcY, viewMatrix, projMatrix);
}

Ray Ray::fromNDC(
    float ndcX, float ndcY,
    const glm::mat4& viewMatrix,
    const glm::mat4& projMatrix) {
    
    // 步骤 1：构造逆变换矩阵
    glm::mat4 invVP = glm::inverse(projMatrix * viewMatrix);
    
    // 步骤 2：NDC → 世界坐标（近平面和远平面各取一点）
    
    // 近平面点（NDC z = -1）
    glm::vec4 nearClip = glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 nearWorld4 = invVP * nearClip;
    glm::vec3 nearWorld = glm::vec3(nearWorld4) / nearWorld4.w;
    
    // 远平面点（NDC z = 1）
    glm::vec4 farClip = glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    glm::vec4 farWorld4 = invVP * farClip;
    glm::vec3 farWorld = glm::vec3(farWorld4) / farWorld4.w;
    
    // 步骤 3：构造射线
    glm::vec3 origin = nearWorld;
    glm::vec3 direction = glm::normalize(farWorld - nearWorld);
    
    return Ray(origin, direction);
}

// ============================================
// 射线上的点计算
// ============================================

glm::vec3 Ray::pointAt(float t) const {
    return origin_ + t * direction_;
}

glm::vec3 Ray::closestPointTo(const glm::vec3& point) const {
    float t = projectPoint(point);
    t = std::max(0.0f, t);  // 限制在射线前方
    return pointAt(t);
}

float Ray::distanceToPoint(const glm::vec3& point) const {
    glm::vec3 closest = closestPointTo(point);
    return glm::distance(point, closest);
}

float Ray::projectPoint(const glm::vec3& point) const {
    glm::vec3 v = point - origin_;
    return glm::dot(v, direction_);
}

// ============================================
// 射线与平面求交
// ============================================

bool Ray::intersectPlane(
    const glm::vec3& planePoint,
    const glm::vec3& planeNormal,
    glm::vec3& outIntersection,
    float* outDistance) const {
    
    // 计算分母
    float denom = glm::dot(planeNormal, direction_);
    
    // 检查平行
    const float EPSILON = 1e-6f;
    if (std::abs(denom) < EPSILON) {
        return false;
    }
    
    // 计算参数 t
    float t = glm::dot(planePoint - origin_, planeNormal) / denom;
    
    // 检查方向
    if (t < 0.0f) {
        return false;
    }
    
    // 计算交点
    outIntersection = pointAt(t);
    
    if (outDistance) {
        *outDistance = t;
    }
    
    return true;
}

bool Ray::intersectXYPlane(
    float z,
    glm::vec3& outIntersection,
    float* outDistance) const {
    
    return intersectPlane(
        glm::vec3(0.0f, 0.0f, z),
        glm::vec3(0.0f, 0.0f, 1.0f),
        outIntersection,
        outDistance);
}

bool Ray::intersectXZPlane(
    float y,
    glm::vec3& outIntersection,
    float* outDistance) const {
    
    return intersectPlane(
        glm::vec3(0.0f, y, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f),
        outIntersection,
        outDistance);
}

bool Ray::intersectYZPlane(
    float x,
    glm::vec3& outIntersection,
    float* outDistance) const {
    
    return intersectPlane(
        glm::vec3(x, 0.0f, 0.0f),
        glm::vec3(1.0f, 0.0f, 0.0f),
        outIntersection,
        outDistance);
}

// ============================================
// 射线与基本几何体求交
// ============================================

bool Ray::intersectLineSegment(
    const glm::vec3& lineStart,
    const glm::vec3& lineEnd,
    glm::vec3& outIntersection,
    float threshold) const {
    
    glm::vec3 lineDir = lineEnd - lineStart;
    float lineLength = glm::length(lineDir);
    
    if (lineLength < 1e-6f) {
        // 退化为点
        float dist = distanceToPoint(lineStart);
        if (dist < threshold) {
            outIntersection = lineStart;
            return true;
        }
        return false;
    }
    
    lineDir /= lineLength;
    
    // 计算最近点参数
    glm::vec3 w0 = origin_ - lineStart;
    float a = glm::dot(direction_, direction_);
    float b = glm::dot(direction_, lineDir);
    float c = glm::dot(lineDir, lineDir);
    float d = glm::dot(direction_, w0);
    float e = glm::dot(lineDir, w0);
    
    float denom = a * c - b * b;
    if (std::abs(denom) < 1e-6f) {
        // 平行
        float dist = distanceToPoint(lineStart);
        if (dist < threshold) {
            outIntersection = lineStart;
            return true;
        }
        return false;
    }
    
    float sc = (b * e - c * d) / denom;
    float tc = (a * e - b * d) / denom;
    
    // 限制线段参数
    tc = std::clamp(tc, 0.0f, lineLength);
    
    // 计算最近点
    glm::vec3 pointOnRay = pointAt(sc);
    glm::vec3 pointOnLine = lineStart + tc * lineDir;
    
    float dist = glm::distance(pointOnRay, pointOnLine);
    
    if (dist < threshold) {
        outIntersection = pointOnLine;
        return true;
    }
    
    return false;
}

bool Ray::intersectSphere(
    const glm::vec3& center,
    float radius,
    glm::vec3& outIntersection,
    float* outDistance) const {
    
    glm::vec3 oc = origin_ - center;
    float a = glm::dot(direction_, direction_);
    float b = 2.0f * glm::dot(oc, direction_);
    float c = glm::dot(oc, oc) - radius * radius;
    
    float discriminant = b * b - 4 * a * c;
    
    if (discriminant < 0.0f) {
        return false;
    }
    
    float t = (-b - std::sqrt(discriminant)) / (2.0f * a);
    
    if (t < 0.0f) {
        t = (-b + std::sqrt(discriminant)) / (2.0f * a);
        if (t < 0.0f) {
            return false;
        }
    }
    
    outIntersection = pointAt(t);
    
    if (outDistance) {
        *outDistance = t;
    }
    
    return true;
}

bool Ray::intersectAABB(
    const glm::vec3& boxMin,
    const glm::vec3& boxMax,
    glm::vec3& outIntersection,
    float* outDistance) const {
    
    float tmin = 0.0f;
    float tmax = std::numeric_limits<float>::max();
    
    for (int i = 0; i < 3; ++i) {
        if (std::abs(direction_[i]) < 1e-6f) {
            if (origin_[i] < boxMin[i] || origin_[i] > boxMax[i]) {
                return false;
            }
        } else {
            float t1 = (boxMin[i] - origin_[i]) / direction_[i];
            float t2 = (boxMax[i] - origin_[i]) / direction_[i];
            
            if (t1 > t2) std::swap(t1, t2);
            
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
            
            if (tmin > tmax) {
                return false;
            }
        }
    }
    
    if (tmin < 0.0f) {
        return false;
    }
    
    outIntersection = pointAt(tmin);
    
    if (outDistance) {
        *outDistance = tmin;
    }
    
    return true;
}

bool Ray::intersectTriangle(
    const glm::vec3& v0,
    const glm::vec3& v1,
    const glm::vec3& v2,
    glm::vec3& outIntersection,
    float* outDistance) const {
    
    // Möller-Trumbore 算法
    const float EPSILON = 1e-6f;
    
    glm::vec3 edge1 = v1 - v0;
    glm::vec3 edge2 = v2 - v0;
    
    glm::vec3 h = glm::cross(direction_, edge2);
    float a = glm::dot(edge1, h);
    
    if (std::abs(a) < EPSILON) {
        return false;  // 平行
    }
    
    float f = 1.0f / a;
    glm::vec3 s = origin_ - v0;
    float u = f * glm::dot(s, h);
    
    if (u < 0.0f || u > 1.0f) {
        return false;
    }
    
    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(direction_, q);
    
    if (v < 0.0f || u + v > 1.0f) {
        return false;
    }
    
    float t = f * glm::dot(edge2, q);
    
    if (t < EPSILON) {
        return false;
    }
    
    outIntersection = pointAt(t);
    
    if (outDistance) {
        *outDistance = t;
    }
    
    return true;
}

// ============================================
// 工具方法
// ============================================

bool Ray::isValid() const {
    float len = glm::length(direction_);
    return std::abs(len - 1.0f) < 1e-3f;
}

Ray Ray::transform(const glm::mat4& transform) const {
    glm::vec4 newOrigin4 = transform * glm::vec4(origin_, 1.0f);
    glm::vec3 newOrigin = glm::vec3(newOrigin4) / newOrigin4.w;
    
    glm::vec4 newDir4 = transform * glm::vec4(direction_, 0.0f);
    glm::vec3 newDirection = glm::vec3(newDir4);
    
    return Ray(newOrigin, newDirection);
}

bool Ray::isParallelTo(const Ray& other, float epsilon) const {
    float dot = std::abs(glm::dot(direction_, other.direction_));
    return std::abs(dot - 1.0f) < epsilon;
}

float Ray::closestPoints(
    const Ray& other,
    glm::vec3& outPoint1,
    glm::vec3& outPoint2) const {
    
    glm::vec3 w0 = origin_ - other.origin_;
    float a = glm::dot(direction_, direction_);
    float b = glm::dot(direction_, other.direction_);
    float c = glm::dot(other.direction_, other.direction_);
    float d = glm::dot(direction_, w0);
    float e = glm::dot(other.direction_, w0);
    
    float denom = a * c - b * b;
    
    if (std::abs(denom) < 1e-6f) {
        // 平行
        outPoint1 = origin_;
        outPoint2 = other.closestPointTo(origin_);
        return glm::distance(outPoint1, outPoint2);
    }
    
    float sc = (b * e - c * d) / denom;
    float tc = (a * e - b * d) / denom;
    
    outPoint1 = pointAt(sc);
    outPoint2 = other.pointAt(tc);
    
    return glm::distance(outPoint1, outPoint2);
}