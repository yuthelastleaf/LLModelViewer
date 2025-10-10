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

    void resetView();
    void setWireframeMode(bool enabled);
    
    // TODO: 后续添加的接口
    // void loadModel(const QString &filePath);
    // void setCamera(const QVector3D &position, const QVector3D &target);
    // void addLight(const QVector3D &position, const QVector3D &color);

signals:
    void fpsUpdated(int fps);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void updateFPS();
    
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
    
    // FPS计算
    QTimer *fpsTimer;
    QElapsedTimer elapsedTimer;
    int frameCount;
};

#endif // GLWIDGET_H