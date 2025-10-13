#include "GLWidget.h"
#include "Demo.h"
#include <glad/glad.h>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QtMath>
#include <QDebug>

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , currentDemo(nullptr)
    , frameCount(0)
    , deltaTime(0.0f)
    , lastFrameTime(0.0f)
{
    // FPS 计时器
    fpsTimer = new QTimer(this);
    connect(fpsTimer, &QTimer::timeout, this, &GLWidget::updateFPS);
    fpsTimer->start(1000);
    
    elapsedTimer.start();
    frameTimer.start();
    
    // 设置焦点策略以接收键盘事件
    setFocusPolicy(Qt::StrongFocus);
}

GLWidget::~GLWidget()
{
    makeCurrent();
    
    // 清理当前 Demo
    if (currentDemo) {
        currentDemo->cleanup();
    }
    
    doneCurrent();
}

void GLWidget::setDemo(std::unique_ptr<Demo> demo)
{
    makeCurrent();
    
    // 清理旧的 Demo
    if (currentDemo) {
        currentDemo->cleanup();
    }
    
    // 设置新的 Demo
    currentDemo = std::move(demo);
    
    if (currentDemo) {
        // 初始化新的 Demo
        currentDemo->initialize();
        
        // 设置视口尺寸
        currentDemo->viewportWidth = width();
        currentDemo->viewportHeight = height();
        
        qDebug() << "Demo loaded:" << currentDemo->getName();
    }
    
    doneCurrent();
    
    emit demoChanged(currentDemo.get());
    update();
}

void GLWidget::clearDemo()
{
    setDemo(nullptr);
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    
    // 初始化 GLAD
    if (!gladLoadGL()) {
        qCritical() << "Failed to initialize GLAD";
        return;
    }
    
    // 打印 OpenGL 信息
    qDebug() << "========================================";
    qDebug() << "OpenGL Version:" << (const char*)glGetString(GL_VERSION);
    qDebug() << "GLSL Version:" << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    qDebug() << "Renderer:" << (const char*)glGetString(GL_RENDERER);
    qDebug() << "Vendor:" << (const char*)glGetString(GL_VENDOR);
    qDebug() << "========================================";
    
    // 设置 OpenGL 状态
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    
    // 如果已经有 Demo，初始化它
    if (currentDemo) {
        currentDemo->initialize();
    }
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    
    // 更新 Demo 的视口尺寸
    if (currentDemo) {
        currentDemo->viewportWidth = w;
        currentDemo->viewportHeight = h;
    }
}

void GLWidget::paintGL()
{
    // 计算 deltaTime
    float currentFrameTime = frameTimer.elapsed() / 1000.0f;
    deltaTime = currentFrameTime - lastFrameTime;
    lastFrameTime = currentFrameTime;
    
    // 清除缓冲
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // 如果有 Demo，更新并渲染
    if (currentDemo) {
        currentDemo->update(deltaTime);
        currentDemo->render();
    }
    
    frameCount++;
    update(); // 持续刷新
}

// ============================================
// 输入事件 - 转发给当前 Demo
// ============================================

void GLWidget::keyPressEvent(QKeyEvent *event)
{
    if (currentDemo) {
        currentDemo->processKeyPress(event);
    }
    
    QOpenGLWidget::keyPressEvent(event);
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    if (currentDemo) {
        currentDemo->processMousePress(event);
    }
    
    QOpenGLWidget::mousePressEvent(event);
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (currentDemo) {
        currentDemo->processMouseMove(event);
    }
    
    QOpenGLWidget::mouseMoveEvent(event);
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (currentDemo) {
        currentDemo->processMouseRelease(event);
    }
    
    QOpenGLWidget::mouseReleaseEvent(event);
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
    if (currentDemo) {
        currentDemo->processMouseWheel(event);
    }
    
    QOpenGLWidget::wheelEvent(event);
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