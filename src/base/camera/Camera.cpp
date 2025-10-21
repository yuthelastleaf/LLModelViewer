#include "Camera.h"
#include <algorithm>
#include <QDebug>

inline QDebug operator<<(QDebug dbg, const glm::vec3& v)
{
    dbg.nospace() << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return dbg.space();
}


Camera::Camera(CameraType camType) : type(camType) {
    updateCameraVectors();
    if (type == CameraType::ORBIT || type == CameraType::ORTHO_2D) {
        updateOrbitPosition();
    }
    
    // 如果是 ORTHO_2D，自动启用 2D 模式
    if (type == CameraType::ORTHO_2D) {
        is2DMode = true;
    }
}

// ============================================
// 矩阵获取
// ============================================

glm::mat4 Camera::getViewMatrix() const {
    if (type == CameraType::ORBIT || type == CameraType::ORTHO_2D) {
        // qDebug() << "lookat params: " << position << "--" <<  position << "--" <<  up;

        return glm::lookAt(position, target, up);
    }
    else {
        return glm::lookAt(position, position + front, up);
    }
}

glm::mat4 Camera::getBackViewMatrix() const {
    if (type == CameraType::ORBIT || type == CameraType::ORTHO_2D) {
        glm::vec3 toCamera = position - target;
        glm::vec3 backPosition = target - toCamera;
        return glm::lookAt(backPosition, target, up);
    }
    else {
        return glm::lookAt(position, position - front, up);
    }
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    if (type == CameraType::ORTHO_2D) {
        // 2D 正交投影
        float halfHeight = radius;  // 使用 radius 作为视口高度
        float halfWidth = halfHeight * aspectRatio;
        return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane, farPlane);
    }
    else {
        // 3D 透视投影
        return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
    }
}

// ============================================
// 输入处理
// ============================================

void Camera::processMouseMovement(float deltaX, float deltaY) {
    if (type == CameraType::ORTHO_2D) {
        // 2D 模式：不处理旋转，由外部调用 pan2D()
        // pan2D(deltaX, deltaY);
        return;
    }
    
    deltaX *= mouseSensitivity;
    deltaY *= mouseSensitivity;

    if (type == CameraType::ORBIT) {
        yaw += deltaX * 0.01f;
        pitch -= deltaY * 0.01f;
        pitch = std::clamp(pitch, -89.0f, 89.0f);
        updateOrbitPosition();
    }
    else {
        yaw += deltaX * 0.01f;
        pitch += deltaY * 0.01f;
        pitch = std::clamp(pitch, -89.0f, 89.0f);
        updateCameraVectors();
    }
}

void Camera::processMouseScroll(float deltaY) {
    if (type == CameraType::ORTHO_2D) {
        // 2D 模式：调整正交视口大小（zoom）
        radius *= (1.0f - deltaY * zoomSpeed);
        radius = std::clamp(radius, 0.1f, 100.0f);
        updateOrbitPosition();
        // qDebug() << "2D Zoom: radius =" << radius;
    }
    else if (type == CameraType::ORBIT) {
        // 3D 轨道模式：调整距离
        radius -= deltaY * scrollSensitivity;
        radius = std::clamp(radius, 1.0f, 50.0f);
        updateOrbitPosition();
    }
    else {
        // FPS/FREE 模式：调整 FOV
        fov -= deltaY;
        fov = std::clamp(fov, 1.0f, 90.0f);
    }
}

void Camera::processKeyboard(CameraMovement direction, float deltaTime) {
    float velocity = moveSpeed * deltaTime;

    if (type == CameraType::ORBIT || type == CameraType::ORTHO_2D) {
        // 轨道/2D 模式：移动目标点
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
        case CameraMovement::RESET:
            reset();
            return;
        }
        updateOrbitPosition();
    }
    else {
        // FPS/自由相机：移动相机位置
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
        case CameraMovement::RESET:
            reset();
            return;
        }
    }
}

void Camera::pan2D(float deltaX, float deltaY, float worldPerPixel) {
    if (!is2DMode) {
        qWarning() << "pan2D() called but not in 2D mode";
        return;
    }
    
    // 计算平移向量（屏幕空间转世界空间）
    glm::vec3 panOffset = -right * deltaX * worldPerPixel * panSensitivity
                        + up * deltaY * worldPerPixel * panSensitivity;
    
    // 同时移动 target 和 position
    target += panOffset;
    position += panOffset;
    
    // qDebug() << "2D Pan: target =" << target.x << target.y << target.z;
}

void Camera::ProcessKeyboard(int direction, float deltaTime) {
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

// ============================================
// 模式设置
// ============================================

void Camera::SetPosition(const glm::vec3 &newPosition) {
    position = newPosition;
    defaultPosition = newPosition;
}

void Camera::SetTarget(const glm::vec3 &newTarget) {
    target = newTarget;
    defaultTarget = newTarget;
    if (type == CameraType::ORBIT || type == CameraType::ORTHO_2D) {
        updateOrbitPosition();
    }
}

void Camera::SetType(CameraType newType) {
    CameraType oldType = type;
    type = newType;
    
    // 自动处理 2D 模式标志
    if (newType == CameraType::ORTHO_2D) {
        is2DMode = true;
    } else if (oldType == CameraType::ORTHO_2D) {
        is2DMode = false;
    }
    
    if (type == CameraType::ORBIT || type == CameraType::ORTHO_2D) {
        updateOrbitPosition();
    }
    else {
        updateCameraVectors();
    }
    
    qDebug() << "Camera type changed to:" << (int)newType << ", 2D mode:" << is2DMode;
}

void Camera::set2DMode(bool enable) {
    if (is2DMode == enable) return;
    
    is2DMode = enable;
    
    if (enable) {
        // 切换到 2D 模式
        qDebug() << "Switching to 2D mode";
        
        // 保存当前 3D 参数
        saved3DYaw = yaw;
        saved3DPitch = pitch;
        saved3DRadius = radius;
        
        // 应用 2D 视图方向
        type = CameraType::ORTHO_2D;
        apply2DOrientation();
    }
    else {
        // 切换回 3D 模式
        qDebug() << "Switching to 3D mode";
        
        // 恢复 3D 参数
        type = CameraType::ORBIT;
        yaw = saved3DYaw;
        pitch = saved3DPitch;
        radius = saved3DRadius;
        
        updateOrbitPosition();
    }
}

void Camera::set2DOrientation(View2DOrientation orientation) {
    view2DOrientation = orientation;
    if (is2DMode) {
        apply2DOrientation();
    }
}

// ============================================
// 预设视图
// ============================================

void Camera::SetOrbitParams(float newRadius, float newYaw, float newPitch) {
    radius = newRadius;
    yaw = newYaw;
    pitch = std::clamp(newPitch, -89.0f, 89.0f);
    
    defaultRadius = newRadius;
    defaultYaw = newYaw;
    defaultPitch = newPitch;
    
    if (type == CameraType::ORBIT || type == CameraType::ORTHO_2D) {
        updateOrbitPosition();
    }
}

// 计算平移偏移
glm::vec3 Camera::screenToWorld(float deltaX, float deltaY, float worldPerPixel)
{
    return (-right * deltaX * worldPerPixel * panSensitivity
                        + up * deltaY * worldPerPixel * panSensitivity);
}

void Camera::SetTopView(float distance) {
    view2DOrientation = View2DOrientation::RIGHT;
    SetType(CameraType::ORTHO_2D);
    SetOrbitParams(distance, 90.0f, 0.0f);  // 右视
    is2DMode = true;
}

void Camera::SetFrontView(float distance) {
    view2DOrientation = View2DOrientation::FRONT;
    SetType(CameraType::ORTHO_2D);
    SetOrbitParams(distance, 0.0f, 0.0f);   // 正视
    is2DMode = true;
}

void Camera::SetRightView(float distance) {
    view2DOrientation = View2DOrientation::TOP;
    SetType(CameraType::ORTHO_2D);
    SetOrbitParams(distance, 0.0f, 89.9f);  // 俯视
    is2DMode = true;
}

void Camera::SetIsometricView(float distance) {
    SetType(CameraType::ORBIT);
    SetOrbitParams(distance, 45.0f, 35.264f); // 等轴测
    is2DMode = false;
}

// ============================================
// 重置
// ============================================

void Camera::reset() {
    position = defaultPosition;
    target = defaultTarget;
    radius = defaultRadius;
    yaw = defaultYaw;
    pitch = defaultPitch;
    fov = defaultFov;
    type = defaultType;
    
    if (type == CameraType::ORBIT || type == CameraType::ORTHO_2D) {
        updateOrbitPosition();
    }
    else {
        updateCameraVectors();
    }
    
    qDebug() << "Camera reset";
}

// ============================================
// 内部方法
// ============================================

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

    // qDebug() << "radius: " << radius << "  yaw:" << yaw << "- x = " << x << " pitch:" << pitch << " - y=" << y << " z=" << z;

    position = target + glm::vec3(x, y, z);
    front = glm::normalize(target - position);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::apply2DOrientation() {
    switch (view2DOrientation) {
        case View2DOrientation::TOP:
            yaw = 90.0f;
            pitch = 0.0f;
            break;
            
        case View2DOrientation::FRONT:
            // 正视图：从 +Z 看向 -Z，X 向右，Y 向上
            yaw = 0.0f;
            pitch = 0.0f;
            break;
            
        case View2DOrientation::RIGHT:
            yaw = 0.0f;
            pitch = 89.9f;// 右视图：从 +X 看向 -X，Z 向右，Y 向上
            break;
    }
    
    updateOrbitPosition();
    qDebug() << "Applied 2D orientation:" << (int)view2DOrientation 
             << ", yaw=" << yaw << ", pitch=" << pitch;
}