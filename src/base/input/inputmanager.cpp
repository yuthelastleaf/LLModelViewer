// GLWidget.h
#include "InputManager.h"
class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    // ...
private:
    InputManager input;
    // ...
};

// GLWidget.cpp
void GLWidget::mousePressEvent(QMouseEvent* e)   { setFocus(); input.onMousePress(e);   QOpenGLWidget::mousePressEvent(e); }
void GLWidget::mouseReleaseEvent(QMouseEvent* e) { input.onMouseRelease(e);             QOpenGLWidget::mouseReleaseEvent(e); }
void GLWidget::mouseMoveEvent(QMouseEvent* e)    { input.onMouseMove(e);                QOpenGLWidget::mouseMoveEvent(e); }
void GLWidget::wheelEvent(QWheelEvent* e)        { input.onWheel(e);                    QOpenGLWidget::wheelEvent(e); }
void GLWidget::keyPressEvent(QKeyEvent* e)       { if (!e->isAutoRepeat()) input.onKeyPress(e);  QOpenGLWidget::keyPressEvent(e); }
void GLWidget::keyReleaseEvent(QKeyEvent* e)     { if (!e->isAutoRepeat()) input.onKeyRelease(e);QOpenGLWidget::keyReleaseEvent(e); }

void GLWidget::paintGL() {
    // 1) 帧起：同步输入快照
    input.beginFrame();

    // 2) 计算 deltaTime（你已有）
    calculateDeltaTime();

    // 3) 轮询式读取（类似 glfwGetKey）
    if (input.isKeyDown(Qt::Key_W)) camera.processKeyboard(CameraMovement::FORWARD,  deltaTime);
    if (input.isKeyDown(Qt::Key_S)) camera.processKeyboard(CameraMovement::BACKWARD, deltaTime);
    if (input.isKeyDown(Qt::Key_A)) camera.processKeyboard(CameraMovement::LEFT,     deltaTime);
    if (input.isKeyDown(Qt::Key_D)) camera.processKeyboard(CameraMovement::RIGHT,    deltaTime);
    if (input.isKeyDown(Qt::Key_E)) camera.processKeyboard(CameraMovement::UP,       deltaTime);
    if (input.isKeyDown(Qt::Key_Q)) camera.processKeyboard(CameraMovement::DOWN,     deltaTime);

    // 也可以用边沿：刚按/刚松
    if (input.wasKeyPressed(Qt::Key_R)) { /* 重置相机等 */ }

    // 鼠标移动/滚轮
    auto d = input.mouseDeltaPixels(); // 本帧累计像素增量
    if (d.manhattanLength() > 0.0) { /* 轨道相机旋转等 */ }
    int wheel = input.wheelDeltaY();
    if (wheel != 0) { /* 缩放/推拉 */ }

    // 4) 你的渲染
    // ...
    update(); // 或用 QTimer 固定帧率
}
