#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

/**
 * WorkPlane - 3D 工作平面类
 * 
 * 功能：
 * - 定义 3D 空间中的一个平面
 * - 提供坐标系变换
 * - 支持动态跟随相机
 * - 射线求交
 */
class WorkPlane {
public:
    // ============================================
    // 构造与预设
    // ============================================
    
    WorkPlane();
    
    // 预设平面
    void setXY(float z = 0.0f);           // XY 平面（俯视图）
    void setXZ(float y = 0.0f);           // XZ 平面（正视图）
    void setYZ(float x = 0.0f);           // YZ 平面（侧视图）
    
    // 从相机视角设置（垂直于视线）
    void setFromView(const glm::vec3& cameraPos, 
                     const glm::vec3& cameraFront,
                     const glm::vec3& cameraTarget);
    
    // 自定义设置
    void set(const glm::vec3& origin, 
             const glm::vec3& normal,
             const glm::vec3& xAxis = glm::vec3(1, 0, 0));

    // ============================================
    // 坐标系
    // ============================================
    
    // 获取平面局部坐标系
    glm::vec3 getOrigin() const { return origin_; }
    glm::vec3 getNormal() const { return normal_; }
    glm::vec3 getXAxis() const { return xAxis_; }
    glm::vec3 getYAxis() const { return yAxis_; }
    
    // 获取平面变换矩阵（局部坐标 → 世界坐标）
    glm::mat4 getLocalToWorld() const;
    
    // 获取平面逆变换矩阵（世界坐标 → 局部坐标）
    glm::mat4 getWorldToLocal() const;

    // ============================================
    // 坐标转换
    // ============================================
    
    // 世界坐标转平面局部坐标（投影到平面）
    glm::vec2 worldToLocal(const glm::vec3& worldPos) const;
    
    // 平面局部坐标转世界坐标
    glm::vec3 localToWorld(const glm::vec2& localPos) const;
    glm::vec3 localToWorld(float u, float v) const;

    // ============================================
    // 几何操作
    // ============================================
    
    // 点到平面的距离（带符号）
    float distanceToPoint(const glm::vec3& point) const;
    
    // 点投影到平面
    glm::vec3 projectPoint(const glm::vec3& point) const;
    
    // 射线与平面求交
    bool rayIntersection(const glm::vec3& rayOrigin,
                        const glm::vec3& rayDirection,
                        glm::vec3& intersection,
                        float* t = nullptr) const;

    // ============================================
    // 动态更新
    // ============================================
    
    // 移动平面（保持方向）
    void translate(const glm::vec3& offset);
    
    // 沿法线方向移动
    void moveAlongNormal(float distance);
    
    // 旋转平面
    void rotate(const glm::quat& rotation);
    void rotateAroundAxis(const glm::vec3& axis, float angle);
    
    // 对齐到指定法向量
    void alignToNormal(const glm::vec3& newNormal);

    // ============================================
    // 自动跟随相机
    // ============================================
    
    struct FollowMode {
        bool enabled = false;           // 是否启用跟随
        bool followPosition = true;     // 跟随位置（origin 跟随 target）
        bool followOrientation = false; // 跟随方向（normal 垂直于视线）
        float distanceFromTarget = 0.0f;// 距离目标点的偏移
    };
    
    void setFollowMode(const FollowMode& mode) { followMode_ = mode; }
    FollowMode& getFollowMode() { return followMode_; }
    const FollowMode& getFollowMode() const { return followMode_; }
    
    // 更新跟随（在相机移动后调用）
    void updateFollow(const glm::vec3& cameraPos,
                     const glm::vec3& cameraFront,
                     const glm::vec3& cameraTarget);

    // ============================================
    // 可视化属性
    // ============================================
    
    struct VisualSettings {
        bool visible = true;
        float size = 20.0f;              // 平面显示大小
        float gridSpacing = 1.0f;        // 网格间距
        glm::vec4 color = glm::vec4(0.3f, 0.6f, 0.9f, 0.3f);  // 半透明蓝色
        glm::vec4 gridColor = glm::vec4(0.5f, 0.7f, 1.0f, 0.5f);
        bool showGrid = true;
        bool showAxes = true;
    };
    
    void setVisualSettings(const VisualSettings& settings) { visual_ = settings; }
    VisualSettings& getVisualSettings() { return visual_; }
    const VisualSettings& getVisualSettings() const { return visual_; }

    // ============================================
    // 状态查询
    // ============================================
    
    bool isValid() const;
    bool isHorizontal() const;  // 是否水平（normal 接近 Z 轴）
    bool isVertical() const;    // 是否垂直（normal 接近 XY 平面）

private:
    // ============================================
    // 内部数据
    // ============================================
    
    glm::vec3 origin_;    // 平面上的一点（通常是中心）
    glm::vec3 normal_;    // 平面法向量（单位向量）
    glm::vec3 xAxis_;     // 平面 X 轴（单位向量）
    glm::vec3 yAxis_;     // 平面 Y 轴（单位向量）
    
    FollowMode followMode_;
    VisualSettings visual_;
    
    // 辅助方法
    void updateAxes();    // 根据 normal_ 计算 xAxis_ 和 yAxis_
    void normalize();     // 规范化所有向量
};