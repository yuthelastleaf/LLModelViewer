#include "glwidget.h"
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , cameraDistance(5.0f)
    , cameraRotationX(30.0f)
    , cameraRotationY(45.0f)
    , isRotating(false)
    , wireframeMode(false)
    , frameCount(0)
{
    fpsTimer = new QTimer(this);
    connect(fpsTimer, &QTimer::timeout, this, &GLWidget::updateFPS);
    fpsTimer->start(1000); // 每秒更新一次FPS
    
    elapsedTimer.start();
}

GLWidget::~GLWidget()
{
    makeCurrent();
    // TODO: 清理OpenGL资源
    doneCurrent();
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    
    // 设置OpenGL状态
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    
    // TODO: 初始化着色器、缓冲区等
    // - 创建并编译着色器程序
    // - 创建VAO/VBO
    // - 加载纹理
    // - 设置光照
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    
    projection.setToIdentity();
    projection.perspective(45.0f, float(w) / float(h), 0.1f, 100.0f);
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 设置视图矩阵
    view.setToIdentity();
    view.translate(0.0f, 0.0f, -cameraDistance);
    view.rotate(cameraRotationX, 1.0f, 0.0f, 0.0f);
    view.rotate(cameraRotationY, 0.0f, 1.0f, 0.0f);
    
    model.setToIdentity();
    
    // 设置线框模式
    if (wireframeMode) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    } else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }
    
    // TODO: 渲染场景
    // - 绑定着色器程序
    // - 设置uniform变量(MVP矩阵、光照等)
    // - 绘制模型
    // - 绘制坐标轴、网格等辅助对象
    
    frameCount++;
    update(); // 持续刷新
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        isRotating = true;
        lastMousePos = event->pos();
    }
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (isRotating) {
        QPoint delta = event->pos() - lastMousePos;
        
        cameraRotationY += delta.x() * 0.5f;
        cameraRotationX += delta.y() * 0.5f;
        
        // 限制X轴旋转角度
        cameraRotationX = qBound(-89.0f, cameraRotationX, 89.0f);
        
        lastMousePos = event->pos();
        update();
    }
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
    float delta = event->angleDelta().y() / 120.0f;
    cameraDistance -= delta * 0.5f;
    cameraDistance = qBound(1.0f, cameraDistance, 50.0f);
    
    update();
}

void GLWidget::resetView()
{
    cameraDistance = 5.0f;
    cameraRotationX = 30.0f;
    cameraRotationY = 45.0f;
    update();
}

void GLWidget::setWireframeMode(bool enabled)
{
    wireframeMode = enabled;
    update();
}

void GLWidget::updateFPS()
{
    qint64 elapsed = elapsedTimer.restart();
    int fps = 0;
    if (elapsed > 0) {
        fps = qRound(frameCount * 1000.0 / elapsed);
    }
    frameCount = 0;
    
    emit fpsUpdated(fps);
}