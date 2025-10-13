#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QTimer>
#include <QElapsedTimer>

class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit GLWidget(QWidget *parent = nullptr);
    ~GLWidget();

    // 公开接口
    void resetView();
    void setWireframeMode(bool enabled);
    void loadModel(const QString &filePath);
    
    // Demo 选择
    enum DemoMode {
        NoDemo,
        TriangleDemo,
        TextureDemo,
        ModelDemo
    };
    
    void setDemoMode(DemoMode mode);

signals:
    void fpsUpdated(int fps);
    void statusMessage(const QString &message);

protected:
    // OpenGL 核心函数（Qt 自动管理上下文）
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    
    // 输入事件处理（替代 GLFW 的输入系统）
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void updateFPS();
    void printOpenGLInfo();
    
    // Demo 渲染函数
    void renderTriangleDemo();
    void renderTextureDemo();
    void renderModelDemo();
    
    // 相机参数
    QMatrix4x4 projection;
    QMatrix4x4 view;
    QMatrix4x4 model;
    
    float cameraDistance;
    float cameraRotationX;
    float cameraRotationY;
    
    // 鼠标交互
    QPoint lastMousePos;
    bool isRotating;
    
    // 渲染设置
    bool wireframeMode;
    DemoMode currentDemo;
    
    // FPS 计算
    QTimer *fpsTimer;
    QElapsedTimer elapsedTimer;
    int frameCount;
    
    // TODO: 添加着色器、VAO、VBO 等资源
    // GLuint shaderProgram;
    // GLuint VAO, VBO;
};

#endif // GLWIDGET_H