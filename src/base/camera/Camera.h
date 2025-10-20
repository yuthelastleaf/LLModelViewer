#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class CameraType {
    ORBIT,      // 3D 轨道相机（可旋转查看）
    FPS,        // 第一人称相机
    FREE,       // 自由相机
    ORTHO_2D    // 2D 正交相机（CAD 专用，只能平移和缩放）
};

enum class CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN,
    RESET
};

// 2D 视图方向
enum class View2DOrientation {
    TOP,        // 俯视图 (XY 平面)
    FRONT,      // 正视图 (XZ 平面)
    RIGHT       // 右视图 (YZ 平面)
};

class Camera {
public:
    // ============================================
    // 公共属性
    // ============================================
    
    // 轨道相机参数（3D ORBIT 模式）
    glm::vec3 target = glm::vec3(0.0f);
    float radius = 5.0f;
    float yaw = 45.0f;
    float pitch = 45.0f;

    // FPS/自由相机参数
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    // 相机属性
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    // 控制参数
    float mouseSensitivity = 100.0f;
    float scrollSensitivity = 2.0f;
    float moveSpeed = 2.5f;
    float panSensitivity = 1.0f;    // 2D 平移灵敏度
    float zoomSpeed = 0.1f;          // 2D 缩放速度

    // ✅ 新增：视口信息
    float worldPerPixel = 1.0f;      // 每像素对应的世界单位
    int viewportWidth = 800;         // 视口宽度
    int viewportHeight = 600;        // 视口高度

    CameraType type = CameraType::ORBIT;

    // 2D 模式特有属性
    View2DOrientation view2DOrientation = View2DOrientation::TOP;
    bool is2DMode = false;  // 是否处于 2D 模式

public:
    Camera(CameraType camType = CameraType::ORBIT);

    // ============================================
    // 矩阵获取
    // ============================================
    glm::mat4 getViewMatrix() const;
    glm::mat4 getBackViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;
    
    // 兼容大写命名
    glm::mat4 GetViewMatrix() const { return getViewMatrix(); }
    glm::mat4 GetBackViewMatrix() const { return getBackViewMatrix(); }
    glm::mat4 GetProjectionMatrix(float aspectRatio) const { return getProjectionMatrix(aspectRatio); }

    // ============================================
    // 输入处理 - 自动根据模式处理
    // ============================================
    void processMouseMovement(float deltaX, float deltaY);
    void processMouseScroll(float deltaY);
    void processKeyboard(CameraMovement direction, float deltaTime);
    
    // 2D 专用：平移
    void pan2D(float deltaX, float deltaY, float worldPerPixel);
    
    // 兼容旧接口
    void ProcessMouseMovement(float deltaX, float deltaY) { processMouseMovement(deltaX, deltaY); }
    void ProcessMouseScroll(float deltaY) { processMouseScroll(deltaY); }
    void ProcessKeyboard(int direction, float deltaTime);

    // ============================================
    // 模式设置
    // ============================================
    void SetPosition(const glm::vec3& newPosition);
    void SetTarget(const glm::vec3& newTarget);
    void SetType(CameraType newType);
    
    // 2D 模式相关
    void set2DMode(bool enable);
    void set2DOrientation(View2DOrientation orientation);
    bool is2D() const { return is2DMode; }
    
    // 预设视图
    void SetTopView(float distance = 10.0f);           // 俯视图
    void SetFrontView(float distance = 10.0f);         // 正视图
    void SetRightView(float distance = 10.0f);         // 右视图
    void SetIsometricView(float distance = 10.0f);     // 等轴测视图
    
    // 轨道相机参数设置
    void SetOrbitParams(float newRadius, float newYaw, float newPitch);
    
    // ============================================
    // 获取器
    // ============================================
    CameraType getType() const { return type; }
    CameraType GetType() const { return type; }
    float getFov() const { return fov; }
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getTarget() const { return target; }
    glm::vec3 getFront() const { return front; }
    glm::vec3 getUp() const { return up; }
    glm::vec3 getRight() const { return right; }
    
    // 重置相机到默认状态
    void reset();
    
    // ============================================
    // 公共工具方法
    // ============================================
    void updateOrbitPosition();
    void updateCameraVectors();

private:
    // 默认值（用于重置）
    glm::vec3 defaultPosition = glm::vec3(0.0f, 0.0f, 5.0f);
    glm::vec3 defaultTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    float defaultRadius = 5.0f;
    float defaultYaw = 45.0f;
    float defaultPitch = 45.0f;
    float defaultFov = 45.0f;
    CameraType defaultType = CameraType::ORBIT;
    
    // 2D 模式状态保存（用于切换回 3D）
    float saved3DYaw = 45.0f;
    float saved3DPitch = 45.0f;
    float saved3DRadius = 5.0f;
    
    // 内部辅助方法
    void apply2DOrientation();
};

#endif // CAMERA_H