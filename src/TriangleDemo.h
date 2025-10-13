#ifndef TRIANGLEDEMO_H
#define TRIANGLEDEMO_H

#include "Demo.h"

/**
 * 三角形演示
 * 展示基本的三角形渲染和颜色控制
 */
class TriangleDemo : public Demo
{
    Q_OBJECT

public:
    explicit TriangleDemo(QObject *parent = nullptr);
    ~TriangleDemo() override;

    // 实现基类接口
    void initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void cleanup() override;
    
    QString getName() const override { return "Triangle Demo"; }
    QString getDescription() const override { 
        return "Basic triangle rendering with color controls"; 
    }

    // 重写控制面板
    QWidget* createControlPanel(QWidget *parent = nullptr) override;

private:
    // OpenGL 资源
    unsigned int shaderProgram;
    unsigned int VAO;
    unsigned int VBO;
    
    // 可配置参数
    glm::vec3 triangleColor;
    float rotationAngle;
    float rotationSpeed;
    bool autoRotate;
    
    // 辅助函数
    void setupShaders();
    void setupGeometry();
};

#endif // TRIANGLEDEMO_H