#include "Demo.h"
#include "Camera.h"
#include "LightManager.h"
#include "GridAxisHelper.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>

Demo::Demo(QObject *parent)
    : QObject(parent)
    , camera(std::make_unique<Camera>())
    , lightManager(std::make_unique<LightManager>())
    , gridAxisHelper(std::make_unique<GridAxisHelper>())
    , viewportWidth(800)
    , viewportHeight(600)
    , isMousePressed(false)
{
    // 设置默认相机
    camera->setPosition(glm::vec3(0.0f, 0.0f, 5.0f));
    camera->setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
}

Demo::~Demo()
{
    cleanup();
}

// ============================================
// 默认输入处理实现
// ============================================

void Demo::processKeyPress(QKeyEvent *event)
{
    const float moveSpeed = 0.1f;
    
    switch (event->key()) {
        case Qt::Key_W:
            camera->processKeyboard(CameraMovement::FORWARD, moveSpeed);
            break;
        case Qt::Key_S:
            camera->processKeyboard(CameraMovement::BACKWARD, moveSpeed);
            break;
        case Qt::Key_A:
            camera->processKeyboard(CameraMovement::LEFT, moveSpeed);
            break;
        case Qt::Key_D:
            camera->processKeyboard(CameraMovement::RIGHT, moveSpeed);
            break;
        case Qt::Key_Q:
            camera->processKeyboard(CameraMovement::UP, moveSpeed);
            break;
        case Qt::Key_E:
            camera->processKeyboard(CameraMovement::DOWN, moveSpeed);
            break;
        case Qt::Key_R:
            camera->reset();
            emit statusMessage("Camera reset");
            break;
        default:
            break;
    }
}

void Demo::processMousePress(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isMousePressed = true;
        lastMousePos = event->pos();
    }
}

void Demo::processMouseMove(QMouseEvent *event)
{
    if (isMousePressed) {
        QPoint delta = event->pos() - lastMousePos;
        
        float xOffset = delta.x() * 0.1f;
        float yOffset = -delta.y() * 0.1f; // 反转 Y 轴
        
        camera->processMouseMovement(xOffset, yOffset);
        
        lastMousePos = event->pos();
    }
}

void Demo::processMouseRelease(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isMousePressed = false;
    }
}

void Demo::processMouseWheel(QWheelEvent *event)
{
    float yOffset = event->angleDelta().y() / 120.0f;
    camera->processMouseScroll(yOffset);
}

// ============================================
// 矩阵获取函数
// ============================================

glm::mat4 Demo::getViewMatrix() const
{
    return camera->getViewMatrix();
}

glm::mat4 Demo::getBackViewMatrix() const
{
    // 移除平移分量（用于天空盒）
    glm::mat4 view = camera->getViewMatrix();
    return glm::mat4(glm::mat3(view));
}

glm::mat4 Demo::getProjectionMatrix() const
{
    float aspect = (float)viewportWidth / (float)viewportHeight;
    return glm::perspective(glm::radians(camera->getFov()), aspect, 0.1f, 100.0f);
}

glm::mat4 Demo::getMVPMatrix(const glm::mat4 &model) const
{
    return getProjectionMatrix() * getViewMatrix() * model;
}

// ============================================
// 控制面板创建
// ============================================

QWidget* Demo::createControlPanel(QWidget *parent)
{
    QWidget *panel = new QWidget(parent);
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // 添加通用控件
    layout->addWidget(createCameraControls(panel));
    layout->addWidget(createLightControls(panel));
    layout->addWidget(createGridAxisControls(panel));
    
    // 添加弹性空间
    layout->addStretch();
    
    return panel;
}

QWidget* Demo::createCameraControls(QWidget *parent)
{
    QGroupBox *group = new QGroupBox("Camera Controls", parent);
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    // TODO: 添加相机控件
    // 可以集成之前创建的 CameraControlPanel
    
    QPushButton *resetBtn = new QPushButton("Reset Camera");
    connect(resetBtn, &QPushButton::clicked, [this]() {
        camera->reset();
        emit statusMessage("Camera reset");
        emit parameterChanged();
    });
    
    layout->addWidget(resetBtn);
    layout->addWidget(new QLabel("Use WASD to move"));
    layout->addWidget(new QLabel("Mouse drag to rotate"));
    layout->addWidget(new QLabel("Scroll to zoom"));
    
    return group;
}

QWidget* Demo::createLightControls(QWidget *parent)
{
    QGroupBox *group = new QGroupBox("Light Controls", parent);
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    // TODO: 添加光照控件
    layout->addWidget(new QLabel("Light controls coming soon..."));
    
    return group;
}

QWidget* Demo::createGridAxisControls(QWidget *parent)
{
    QGroupBox *group = new QGroupBox("Grid & Axis", parent);
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    // TODO: 添加网格/坐标轴控件
    layout->addWidget(new QLabel("Grid/Axis controls coming soon..."));
    
    return group;
}