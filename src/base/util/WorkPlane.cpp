#include "WorkPlane.h"
#include <cmath>
#include <QDebug>

WorkPlane::WorkPlane()
    : origin_(0.0f, 0.0f, 0.0f)
    , normal_(0.0f, 0.0f, 1.0f)
    , xAxis_(1.0f, 0.0f, 0.0f)
    , yAxis_(0.0f, 1.0f, 0.0f)
{
}

// ============================================
// 预设平面
// ============================================

void WorkPlane::setXY(float z) {
    origin_ = glm::vec3(0.0f, 0.0f, z);
    normal_ = glm::vec3(0.0f, 0.0f, 1.0f);
    xAxis_ = glm::vec3(1.0f, 0.0f, 0.0f);
    yAxis_ = glm::vec3(0.0f, 1.0f, 0.0f);
}

void WorkPlane::setXZ(float y) {
    origin_ = glm::vec3(0.0f, y, 0.0f);
    normal_ = glm::vec3(0.0f, 1.0f, 0.0f);
    xAxis_ = glm::vec3(1.0f, 0.0f, 0.0f);
    yAxis_ = glm::vec3(0.0f, 0.0f, 1.0f);
}

void WorkPlane::setYZ(float x) {
    origin_ = glm::vec3(x, 0.0f, 0.0f);
    normal_ = glm::vec3(1.0f, 0.0f, 0.0f);
    xAxis_ = glm::vec3(0.0f, 1.0f, 0.0f);
    yAxis_ = glm::vec3(0.0f, 0.0f, 1.0f);
}

void WorkPlane::setFromView(const glm::vec3& cameraPos,
                            const glm::vec3& cameraFront,
                            const glm::vec3& cameraTarget) {
    // 平面法向量 = 相机视线方向（垂直于屏幕）
    normal_ = glm::normalize(cameraFront);
    
    // 平面原点 = 相机目标点
    origin_ = cameraTarget;
    
    // 更新坐标轴
    updateAxes();
    
    qDebug() << "Work plane set from view:"
             << "origin=" << origin_.x << origin_.y << origin_.z
             << "normal=" << normal_.x << normal_.y << normal_.z;
}

void WorkPlane::set(const glm::vec3& origin,
                   const glm::vec3& normal,
                   const glm::vec3& xAxis) {
    origin_ = origin;
    normal_ = glm::normalize(normal);
    xAxis_ = glm::normalize(xAxis);
    
    // 确保 xAxis 垂直于 normal
    xAxis_ = glm::normalize(xAxis_ - glm::dot(xAxis_, normal_) * normal_);
    
    // 计算 yAxis
    yAxis_ = glm::normalize(glm::cross(normal_, xAxis_));
}

// ============================================
// 坐标系
// ============================================

glm::mat4 WorkPlane::getLocalToWorld() const {
    // 构造变换矩阵：[xAxis | yAxis | normal | origin]
    return glm::mat4(
        glm::vec4(xAxis_, 0.0f),
        glm::vec4(yAxis_, 0.0f),
        glm::vec4(normal_, 0.0f),
        glm::vec4(origin_, 1.0f)
    );
}

glm::mat4 WorkPlane::getWorldToLocal() const {
    return glm::inverse(getLocalToWorld());
}

// ============================================
// 坐标转换
// ============================================

glm::vec2 WorkPlane::worldToLocal(const glm::vec3& worldPos) const {
    glm::vec3 offset = worldPos - origin_;
    float u = glm::dot(offset, xAxis_);
    float v = glm::dot(offset, yAxis_);
    return glm::vec2(u, v);
}

glm::vec3 WorkPlane::localToWorld(const glm::vec2& localPos) const {
    return origin_ + localPos.x * xAxis_ + localPos.y * yAxis_;
}

glm::vec3 WorkPlane::localToWorld(float u, float v) const {
    return localToWorld(glm::vec2(u, v));
}

// ============================================
// 几何操作
// ============================================

float WorkPlane::distanceToPoint(const glm::vec3& point) const {
    return glm::dot(point - origin_, normal_);
}

glm::vec3 WorkPlane::projectPoint(const glm::vec3& point) const {
    float dist = distanceToPoint(point);
    return point - dist * normal_;
}

bool WorkPlane::rayIntersection(const glm::vec3& rayOrigin,
                               const glm::vec3& rayDirection,
                               glm::vec3& intersection,
                               float* t) const {
    // 射线方程：P(t) = rayOrigin + t * rayDirection
    // 平面方程：dot(P - origin, normal) = 0
    
    float denom = glm::dot(normal_, rayDirection);
    
    // 检查平行
    if (std::abs(denom) < 1e-6f) {
        return false;
    }
    
    // 计算参数 t
    float tValue = glm::dot(origin_ - rayOrigin, normal_) / denom;
    
    // 检查方向
    if (tValue < 0) {
        return false;  // 交点在射线后方
    }
    
    // 计算交点
    intersection = rayOrigin + tValue * rayDirection;
    
    if (t) {
        *t = tValue;
    }
    
    return true;
}

// ============================================
// 动态更新
// ============================================

void WorkPlane::translate(const glm::vec3& offset) {
    origin_ += offset;
}

void WorkPlane::moveAlongNormal(float distance) {
    origin_ += distance * normal_;
}

void WorkPlane::rotate(const glm::quat& rotation) {
    normal_ = glm::normalize(rotation * normal_);
    xAxis_ = glm::normalize(rotation * xAxis_);
    yAxis_ = glm::normalize(rotation * yAxis_);
}

void WorkPlane::rotateAroundAxis(const glm::vec3& axis, float angle) {
    glm::quat rotation = glm::angleAxis(angle, glm::normalize(axis));
    rotate(rotation);
}

void WorkPlane::alignToNormal(const glm::vec3& newNormal) {
    normal_ = glm::normalize(newNormal);
    updateAxes();
}

// ============================================
// 自动跟随相机
// ============================================

void WorkPlane::updateFollow(const glm::vec3& cameraPos,
                            const glm::vec3& cameraFront,
                            const glm::vec3& cameraTarget) {
    if (!followMode_.enabled) {
        return;
    }
    
    // 跟随位置
    if (followMode_.followPosition) {
        origin_ = cameraTarget;
        
        // 应用偏移
        if (std::abs(followMode_.distanceFromTarget) > 1e-6f) {
            origin_ += normal_ * followMode_.distanceFromTarget;
        }
    }
    
    // 跟随方向
    if (followMode_.followOrientation) {
        normal_ = glm::normalize(cameraFront);
        updateAxes();
    }
}

// ============================================
// 状态查询
// ============================================

bool WorkPlane::isValid() const {
    float len = glm::length(normal_);
    return std::abs(len - 1.0f) < 1e-3f;
}

bool WorkPlane::isHorizontal() const {
    return std::abs(glm::dot(normal_, glm::vec3(0, 0, 1))) > 0.99f;
}

bool WorkPlane::isVertical() const {
    return std::abs(glm::dot(normal_, glm::vec3(0, 0, 1))) < 0.1f;
}

// ============================================
// 内部辅助方法
// ============================================

void WorkPlane::updateAxes() {
    // 选择一个不平行于 normal 的向量作为参考
    glm::vec3 reference;
    if (std::abs(normal_.z) < 0.9f) {
        reference = glm::vec3(0, 0, 1);  // 大多数情况用 Z 轴
    } else {
        reference = glm::vec3(1, 0, 0);  // normal 接近 Z 轴时用 X 轴
    }
    
    // 构造正交基
    xAxis_ = glm::normalize(glm::cross(reference, normal_));
    yAxis_ = glm::normalize(glm::cross(normal_, xAxis_));
}

void WorkPlane::normalize() {
    normal_ = glm::normalize(normal_);
    xAxis_ = glm::normalize(xAxis_);
    yAxis_ = glm::normalize(yAxis_);
}