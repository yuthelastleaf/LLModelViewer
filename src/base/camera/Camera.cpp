#include "Camera.h"
#include <algorithm>

Camera::Camera(CameraType camType) : type(camType) {
    updateCameraVectors();
    if (type == CameraType::ORBIT) {
        updateOrbitPosition();
    }
}

glm::mat4 Camera::getViewMatrix() const {
    if (type == CameraType::ORBIT) {
        return glm::lookAt(position, target, up);
    }
    else {
        return glm::lookAt(position, position + front, up);
    }
}

glm::mat4 Camera::getBackViewMatrix() const {
    if (type == CameraType::ORBIT) {
        // 轨道相机：镜像位置法
        glm::vec3 toCamera = position - target;
        glm::vec3 backPosition = target - toCamera;
        return glm::lookAt(backPosition, target, up);
    }
    else {
        // 自由相机：反向front向量
        return glm::lookAt(position, position - front, up);
    }
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

void Camera::processMouseMovement(float deltaX, float deltaY) {
    deltaX *= mouseSensitivity;
    deltaY *= mouseSensitivity;

    if (type == CameraType::ORBIT) {
        yaw += deltaX * 0.01f;
        pitch -= deltaY * 0.01f;

        // 限制pitch角度
        pitch = std::clamp(pitch, -89.0f, 89.0f);

        updateOrbitPosition();
    }
    else {
        yaw += deltaX * 0.01f;
        pitch += deltaY * 0.01f;

        // 限制pitch角度
        pitch = std::clamp(pitch, -89.0f, 89.0f);

        updateCameraVectors();
    }
}

void Camera::processMouseScroll(float deltaY) {
    if (type == CameraType::ORBIT) {
        radius -= deltaY * scrollSensitivity;
        radius = std::clamp(radius, 1.0f, 50.0f);
        updateOrbitPosition();
    }
    else {
        fov -= deltaY;
        fov = std::clamp(fov, 1.0f, 90.0f);
    }
}

void Camera::processKeyboard(CameraMovement direction, float deltaTime) {
    float velocity = moveSpeed * deltaTime;

    if (type == CameraType::ORBIT) {
        // 轨道相机移动目标点
        switch (direction) {
        case CameraMovement::FORWARD:
            target += front * velocity;
            break;
        case CameraMovement::BACKWARD:
            target -= front * velocity;
            break;
        case CameraMovement::LEFT:
            target -= right * velocity;
            break;
        case CameraMovement::RIGHT:
            target += right * velocity;
            break;
        case CameraMovement::UP:
            target += worldUp * velocity;
            break;
        case CameraMovement::DOWN:
            target -= worldUp * velocity;
            break;
        }
        updateOrbitPosition();
    }
    else {
        // FPS/自由相机移动相机位置
        switch (direction) {
        case CameraMovement::FORWARD:
            position += front * velocity;
            break;
        case CameraMovement::BACKWARD:
            position -= front * velocity;
            break;
        case CameraMovement::LEFT:
            position -= right * velocity;
            break;
        case CameraMovement::RIGHT:
            position += right * velocity;
            break;
        case CameraMovement::UP:
            position += worldUp * velocity;
            break;
        case CameraMovement::DOWN:
            position -= worldUp * velocity;
            break;
        }
    }
}

void Camera::ProcessKeyboard(int direction, float deltaTime) {
    // 整数版本兼容接口
    CameraMovement move;
    switch (direction) {
        case 0: move = CameraMovement::FORWARD; break;
        case 1: move = CameraMovement::LEFT; break;
        case 2: move = CameraMovement::BACKWARD; break;
        case 3: move = CameraMovement::RIGHT; break;
        default: return;
    }
    processKeyboard(move, deltaTime);
}

void Camera::SetPosition(const glm::vec3 &newPosition)
{
    position = newPosition;
    defaultPosition = newPosition; // 更新默认值
}

void Camera::SetTarget(const glm::vec3 &newTarget)
{
    target = newTarget;
    defaultTarget = newTarget; // 更新默认值
    if (type == CameraType::ORBIT) {
        updateOrbitPosition();
    }
}

void Camera::SetType(CameraType newType) {
    type = newType;
    if (type == CameraType::ORBIT) {
        updateOrbitPosition();
    }
    else {
        updateCameraVectors();
    }
}

void Camera::reset() {
    // 重置到默认状态
    position = defaultPosition;
    target = defaultTarget;
    radius = defaultRadius;
    yaw = defaultYaw;
    pitch = defaultPitch;
    fov = defaultFov;
    
    if (type == CameraType::ORBIT) {
        updateOrbitPosition();
    }
    else {
        updateCameraVectors();
    }
}

void Camera::updateCameraVectors() {
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::updateOrbitPosition() {
    float x = radius * cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    float y = radius * sin(glm::radians(pitch));
    float z = radius * cos(glm::radians(pitch)) * sin(glm::radians(yaw));

    position = target + glm::vec3(x, y, z);
    front = glm::normalize(target - position);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}