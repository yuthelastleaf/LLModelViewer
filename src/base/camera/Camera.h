#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class CameraType {
    ORBIT,      // 轨道相机
    FPS,        // 第一人称相机
    FREE        // 自由相机
};

enum class CameraMovement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    UP,
    DOWN
};

class Camera {
public:
    // 轨道相机参数
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

    CameraType type = CameraType::ORBIT;

public:
    Camera(CameraType camType = CameraType::ORBIT);

    // 矩阵获取 - 使用小写命名以匹配 Demo 基类
    glm::mat4 getViewMatrix() const;
    glm::mat4 getBackViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;
    
    // 兼容大写命名（保留以防其他地方使用）
    glm::mat4 GetViewMatrix() const { return getViewMatrix(); }
    glm::mat4 GetBackViewMatrix() const { return getBackViewMatrix(); }
    glm::mat4 GetProjectionMatrix(float aspectRatio) const { return getProjectionMatrix(aspectRatio); }

    // 输入处理 - 使用小写命名以匹配 Demo 基类
    void processMouseMovement(float deltaX, float deltaY);
    void processMouseScroll(float deltaY);
    void processKeyboard(CameraMovement direction, float deltaTime);
    
    // 兼容旧接口
    void ProcessMouseMovement(float deltaX, float deltaY) { processMouseMovement(deltaX, deltaY); }
    void ProcessMouseScroll(float deltaY) { processMouseScroll(deltaY); }
    void ProcessKeyboard(int direction, float deltaTime); // 保留整数版本

    // 设置器
    void SetPosition(const glm::vec3& newPosition);
    void SetTarget(const glm::vec3& newTarget);
    void SetType(CameraType newType);
    
    // 获取器
    CameraType GetType() const { return type; }
    float getFov() const { return fov; }
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getTarget() const { return target; }
    glm::vec3 getFront() const { return front; }
    glm::vec3 getUp() const { return up; }
    glm::vec3 getRight() const { return right; }
    
    // 重置相机到默认状态
    void reset();
    
private:
    // 默认值（用于重置）
    glm::vec3 defaultPosition = glm::vec3(0.0f, 0.0f, 5.0f);
    glm::vec3 defaultTarget = glm::vec3(0.0f, 0.0f, 0.0f);
    float defaultRadius = 5.0f;
    float defaultYaw = 45.0f;
    float defaultPitch = 45.0f;
    float defaultFov = 45.0f;
    
    void updateCameraVectors();
    void updateOrbitPosition();
};

#endif // CAMERA_H