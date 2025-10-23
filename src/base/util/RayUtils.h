#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <limits>

/**
 * Ray - 纯粹的 3D 射线类（无外部依赖）
 * 
 * 设计原则：
 * - 只依赖 glm 数学库
 * - 不依赖任何业务类（Camera, WorkPlane, Renderer 等）
 * - 提供纯粹的几何计算功能
 * - 所有功能通过参数传递
 */
class Ray {
public:
    // ============================================
    // 构造函数
    // ============================================
    
    /**
     * 默认构造（指向 -Z 方向的射线）
     */
    Ray();
    
    /**
     * 直接构造射线
     * @param origin 射线起点
     * @param direction 射线方向（会自动归一化）
     */
    Ray(const glm::vec3& origin, const glm::vec3& direction);

    // ============================================
    // 静态工厂方法：从屏幕坐标生成射线
    // ============================================
    
    /**
     * 从屏幕坐标生成射线（核心方法）
     * 
     * @param screenX 屏幕 X 坐标（像素，左上角为原点）
     * @param screenY 屏幕 Y 坐标（像素，左上角为原点）
     * @param viewportWidth 视口宽度（像素）
     * @param viewportHeight 视口高度（像素）
     * @param viewMatrix 视图矩阵（相机变换）
     * @param projMatrix 投影矩阵（透视或正交）
     * @return 生成的射线
     */
    static Ray fromScreen(
        int screenX, int screenY,
        int viewportWidth, int viewportHeight,
        const glm::mat4& viewMatrix,
        const glm::mat4& projMatrix);
    
    /**
     * 从 NDC 坐标生成射线
     * @param ndcX NDC X 坐标 [-1, 1]
     * @param ndcY NDC Y 坐标 [-1, 1]
     * @param viewMatrix 视图矩阵
     * @param projMatrix 投影矩阵
     * @return 生成的射线
     */
    static Ray fromNDC(
        float ndcX, float ndcY,
        const glm::mat4& viewMatrix,
        const glm::mat4& projMatrix);

    // ============================================
    // 访问器
    // ============================================
    
    glm::vec3 getOrigin() const { return origin_; }
    glm::vec3 getDirection() const { return direction_; }
    
    void setOrigin(const glm::vec3& origin) { origin_ = origin; }
    void setDirection(const glm::vec3& direction) { 
        direction_ = glm::normalize(direction); 
    }

    // ============================================
    // 射线上的点计算
    // ============================================
    
    /**
     * 计算射线上距离起点 t 单位的点
     * P(t) = origin + t * direction
     * 
     * @param t 距离参数（t >= 0 为射线前方）
     * @return 射线上的点
     */
    glm::vec3 pointAt(float t) const;
    
    /**
     * 计算射线上最接近给定点的位置
     * @param point 目标点
     * @return 射线上最近的点
     */
    glm::vec3 closestPointTo(const glm::vec3& point) const;
    
    /**
     * 计算点到射线的最短距离
     * @param point 目标点
     * @return 最短距离
     */
    float distanceToPoint(const glm::vec3& point) const;
    
    /**
     * 计算点在射线上的投影参数 t
     * @param point 目标点
     * @return 参数 t（可能为负）
     */
    float projectPoint(const glm::vec3& point) const;

    // ============================================
    // 射线与平面求交
    // ============================================
    
    /**
     * 与任意平面求交（核心方法）
     * 
     * 平面定义：所有满足 dot(P - planePoint, planeNormal) = 0 的点 P
     * 
     * @param planePoint 平面上的任意一点
     * @param planeNormal 平面法向量（必须归一化）
     * @param outIntersection [输出] 交点坐标
     * @param outDistance [输出] 距离参数 t（可选）
     * @return 是否相交（false = 平行或在后方）
     */
    bool intersectPlane(
        const glm::vec3& planePoint,
        const glm::vec3& planeNormal,
        glm::vec3& outIntersection,
        float* outDistance = nullptr) const;
    
    /**
     * 与 XY 平面求交（Z = constant）
     * @param z 平面的 Z 坐标
     * @param outIntersection [输出] 交点
     * @param outDistance [输出] 距离（可选）
     * @return 是否相交
     */
    bool intersectXYPlane(
        float z,
        glm::vec3& outIntersection,
        float* outDistance = nullptr) const;
    
    /**
     * 与 XZ 平面求交（Y = constant）
     */
    bool intersectXZPlane(
        float y,
        glm::vec3& outIntersection,
        float* outDistance = nullptr) const;
    
    /**
     * 与 YZ 平面求交（X = constant）
     */
    bool intersectYZPlane(
        float x,
        glm::vec3& outIntersection,
        float* outDistance = nullptr) const;

    // ============================================
    // 射线与基本几何体求交
    // ============================================
    
    /**
     * 与线段求交（用于拾取/选择）
     * 
     * @param lineStart 线段起点
     * @param lineEnd 线段终点
     * @param outIntersection [输出] 最近点（线段上）
     * @param threshold 距离阈值（射线到线段的最大距离）
     * @return 是否在阈值范围内相交
     */
    bool intersectLineSegment(
        const glm::vec3& lineStart,
        const glm::vec3& lineEnd,
        glm::vec3& outIntersection,
        float threshold = 0.1f) const;
    
    /**
     * 与球体求交
     * 
     * @param center 球心
     * @param radius 半径
     * @param outIntersection [输出] 第一个交点（近点）
     * @param outDistance [输出] 距离（可选）
     * @return 是否相交
     */
    bool intersectSphere(
        const glm::vec3& center,
        float radius,
        glm::vec3& outIntersection,
        float* outDistance = nullptr) const;
    
    /**
     * 与轴对齐包围盒（AABB）求交
     * 
     * @param boxMin 包围盒最小点
     * @param boxMax 包围盒最大点
     * @param outIntersection [输出] 第一个交点
     * @param outDistance [输出] 距离（可选）
     * @return 是否相交
     */
    bool intersectAABB(
        const glm::vec3& boxMin,
        const glm::vec3& boxMax,
        glm::vec3& outIntersection,
        float* outDistance = nullptr) const;
    
    /**
     * 与三角形求交
     * 
     * @param v0 三角形顶点 0
     * @param v1 三角形顶点 1
     * @param v2 三角形顶点 2
     * @param outIntersection [输出] 交点
     * @param outDistance [输出] 距离（可选）
     * @return 是否相交
     */
    bool intersectTriangle(
        const glm::vec3& v0,
        const glm::vec3& v1,
        const glm::vec3& v2,
        glm::vec3& outIntersection,
        float* outDistance = nullptr) const;

    // ============================================
    // 工具方法
    // ============================================
    
    /**
     * 判断射线是否有效（方向是否归一化）
     */
    bool isValid() const;
    
    /**
     * 射线变换（应用矩阵变换）
     * @param transform 4x4 变换矩阵
     * @return 变换后的射线
     */
    Ray transform(const glm::mat4& transform) const;
    
    /**
     * 判断两条射线是否平行
     * @param other 另一条射线
     * @param epsilon 容差
     * @return 是否平行
     */
    bool isParallelTo(const Ray& other, float epsilon = 1e-6f) const;
    
    /**
     * 计算两条射线的最近点对
     * @param other 另一条射线
     * @param outPoint1 [输出] 本射线上的最近点
     * @param outPoint2 [输出] 另一射线上的最近点
     * @return 最短距离
     */
    float closestPoints(
        const Ray& other,
        glm::vec3& outPoint1,
        glm::vec3& outPoint2) const;

private:
    glm::vec3 origin_;       // 射线起点
    glm::vec3 direction_;    // 射线方向（归一化）
};