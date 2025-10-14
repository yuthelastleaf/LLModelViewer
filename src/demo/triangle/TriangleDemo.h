#ifndef TRIANGLEDEMO_H
#define TRIANGLEDEMO_H

#include "../../base/Demo.h"
#include <memory>

// 前向声明
class Shader;

/**
 * TriangleDemo - 基础三角形演示
 * 
 * 功能：
 * - 显示一个旋转的彩色三角形
 * - 演示基本的 VAO/VBO 使用
 * - 演示着色器的基本使用
 * - 演示简单的动画
 */
class TriangleDemo : public Demo
{
    Q_OBJECT

public:
    explicit TriangleDemo(QObject *parent = nullptr);
    ~TriangleDemo() override;

    // ============================================
    // Demo 基本信息
    // ============================================
    
    QString getName() const override {
        return "Triangle Demo";
    }
    
    QString getDescription() const override {
        return "A simple rotating colored triangle.\n\n"
               "This demo demonstrates:\n"
               "• Basic vertex buffer objects (VBO)\n"
               "• Vertex array objects (VAO)\n"
               "• Simple vertex and fragment shaders\n"
               "• Basic animation with rotation\n"
               "• Color interpolation";
    }

    // ============================================
    // Demo 生命周期
    // ============================================
    
    void initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void cleanup() override;

    // ============================================
    // 控制面板
    // ============================================
    
    QWidget* createControlPanel(QWidget *parent = nullptr) override;

private slots:
    void onRotationSpeedChanged(double value);
    void onAutoRotateChanged(bool enabled);
    void onResetRotation();

private:
    // OpenGL 资源
    std::unique_ptr<Shader> shader;
    uint VAO;
    uint VBO;
    
    // 动画参数
    float rotation;          // 当前旋转角度
    float rotationSpeed;     // 旋转速度（度/秒）
    bool autoRotate;         // 是否自动旋转
    
    // 三角形大小
    float triangleScale;
};

#endif // TRIANGLEDEMO_H