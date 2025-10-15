#include "TriangleDemo.h"
#include "../../base/util/shader.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QCheckBox>
#include <QTimer>
#include <QDebug>
// 移除 #include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

TriangleDemo::TriangleDemo(QObject *parent)
    : Demo(parent)
    , VAO(0)
    , VBO(0)
    , rotation(0.0f)
    , rotationSpeed(45.0f)  // 每秒旋转 45 度
    , autoRotate(true)
    , triangleScale(0.5f)
{
    qDebug() << "TriangleDemo created";
}

TriangleDemo::~TriangleDemo()
{
    qDebug() << "TriangleDemo destroying...";
    cleanup();
}

void TriangleDemo::initialize()
{
    // 初始化 Qt 的 OpenGL 函数（必须在使用任何 OpenGL 函数前调用）
    if (!initializeOpenGLFunctions()) {
        qCritical() << "Failed to initialize OpenGL functions in TriangleDemo!";
        return;
    }
    
    qDebug() << "TriangleDemo: OpenGL functions initialized";
    qDebug() << "TriangleDemo: Reading shaders...";
    
    // 创建着色器
    try {
        shader = std::make_unique<Shader>(
            "shaders/triangle/triangle.vs",
            "shaders/triangle/triangle.fs"
        );
        qDebug() << "TriangleDemo: Shaders created successfully";
    } catch (const std::exception& e) {
        qCritical() << "Failed to create shader:" << e.what();
        return;
    }
    
    // 定义三角形顶点数据
    // 每个顶点: 位置(x,y,z) + 颜色(r,g,b)
    float vertices[] = {
        // 位置              // 颜色
        -0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,  // 左下 - 红色
         0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,  // 右下 - 绿色
         0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f   // 顶部 - 蓝色
    };
    
    qDebug() << "TriangleDemo: Generating VAO and VBO...";

    // 生成并绑定 VAO
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    // 生成并绑定 VBO
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    // 设置顶点属性指针
    // 位置属性 (location = 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    // 颜色属性 (location = 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    
    // 解绑
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    
    qDebug() << "TriangleDemo: VAO =" << VAO << ", VBO =" << VBO;
    
    // 检查 OpenGL 错误
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        qCritical() << "OpenGL error during initialization:" << err;
    } else {
        qDebug() << "TriangleDemo: Initialization complete, no OpenGL errors";
    }
    
    emit statusMessage("Triangle Demo initialized");
}

void TriangleDemo::update(float deltaTime)
{
    // 更新旋转角度
    if (autoRotate) {
        rotation += rotationSpeed * deltaTime;
        
        // 保持角度在 0-360 范围内
        if (rotation >= 360.0f) {
            rotation -= 360.0f;
        }
    }
}

void TriangleDemo::render()
{
    // 检查着色器是否有效
    if (!shader) {
        qWarning() << "TriangleDemo: Shader is null, cannot render";
        return;
    }
    
    // 检查 VAO 是否有效
    if (VAO == 0) {
        qWarning() << "TriangleDemo: VAO is 0, cannot render";
        return;
    }
    
    // 使用着色器
    shader->use();
    
    // 构建变换矩阵
    glm::mat4 model = glm::mat4(1.0f);
    
    // 应用缩放
    model = glm::scale(model, glm::vec3(triangleScale));
    
    // 应用旋转（绕 Z 轴）
    model = glm::rotate(model, glm::radians(rotation), glm::vec3(0.0f, 0.0f, 1.0f));
    
    // 获取 MVP 矩阵
    glm::mat4 view = getViewMatrix();
    glm::mat4 projection = getProjectionMatrix();
    glm::mat4 mvp = projection * view * model;
    
    // 传递 MVP 矩阵到着色器
    shader->setMat4("mvp", mvp);
    
    // 绘制三角形
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    
    // 检查 OpenGL 错误（仅在调试时启用）
    #ifdef _DEBUG
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        qWarning() << "OpenGL error in render:" << err;
    }
    #endif
}

void TriangleDemo::cleanup()
{
    qDebug() << "TriangleDemo: Cleaning up resources...";
    
    if (VAO != 0) {
        glDeleteVertexArrays(1, &VAO);
        qDebug() << "TriangleDemo: Deleted VAO" << VAO;
        VAO = 0;
    }
    
    if (VBO != 0) {
        glDeleteBuffers(1, &VBO);
        qDebug() << "TriangleDemo: Deleted VBO" << VBO;
        VBO = 0;
    }
    
    if (shader) {
        shader.reset();
        qDebug() << "TriangleDemo: Shader reset";
    }
    
    qDebug() << "TriangleDemo: Cleanup complete";
}

// ============================================
// 控制面板
// ============================================

QWidget* TriangleDemo::createControlPanel(QWidget *parent)
{
    QWidget *panel = new QWidget(parent);
    QVBoxLayout *mainLayout = new QVBoxLayout(panel);
    
    // 三角形控制组
    QGroupBox *triangleGroup = new QGroupBox("Triangle Controls");
    QVBoxLayout *triangleLayout = new QVBoxLayout(triangleGroup);
    
    // 自动旋转复选框
    QCheckBox *autoRotateCheckBox = new QCheckBox("Auto Rotate");
    autoRotateCheckBox->setChecked(autoRotate);
    connect(autoRotateCheckBox, &QCheckBox::toggled, this, &TriangleDemo::onAutoRotateChanged);
    triangleLayout->addWidget(autoRotateCheckBox);
    
    // 旋转速度控制
    QHBoxLayout *speedLayout = new QHBoxLayout();
    speedLayout->addWidget(new QLabel("Rotation Speed:"));
    QDoubleSpinBox *speedSpinBox = new QDoubleSpinBox();
    speedSpinBox->setRange(-360.0, 360.0);
    speedSpinBox->setSingleStep(5.0);
    speedSpinBox->setValue(rotationSpeed);
    speedSpinBox->setSuffix(" °/s");
    connect(speedSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &TriangleDemo::onRotationSpeedChanged);
    speedLayout->addWidget(speedSpinBox);
    triangleLayout->addLayout(speedLayout);
    
    // 当前旋转角度显示
    QHBoxLayout *angleLayout = new QHBoxLayout();
    angleLayout->addWidget(new QLabel("Current Angle:"));
    QLabel *angleLabel = new QLabel(QString::number(rotation, 'f', 1) + "°");
    angleLayout->addWidget(angleLabel);
    angleLayout->addStretch();
    
    // 定时更新角度显示
    QTimer *angleTimer = new QTimer(panel);
    connect(angleTimer, &QTimer::timeout, [this, angleLabel]() {
        angleLabel->setText(QString::number(rotation, 'f', 1) + "°");
    });
    angleTimer->start(100); // 每 100ms 更新一次
    
    triangleLayout->addLayout(angleLayout);
    
    // 重置旋转按钮
    QPushButton *resetButton = new QPushButton("Reset Rotation");
    connect(resetButton, &QPushButton::clicked, this, &TriangleDemo::onResetRotation);
    triangleLayout->addWidget(resetButton);
    
    // 缩放控制
    QHBoxLayout *scaleLayout = new QHBoxLayout();
    scaleLayout->addWidget(new QLabel("Scale:"));
    QSlider *scaleSlider = new QSlider(Qt::Horizontal);
    scaleSlider->setRange(10, 200);
    scaleSlider->setValue(static_cast<int>(triangleScale * 100));
    QLabel *scaleLabel = new QLabel(QString::number(triangleScale, 'f', 2));
    connect(scaleSlider, &QSlider::valueChanged, [this, scaleLabel](int value) {
        triangleScale = value / 100.0f;
        scaleLabel->setText(QString::number(triangleScale, 'f', 2));
        emit parameterChanged();
    });
    scaleLayout->addWidget(scaleSlider);
    scaleLayout->addWidget(scaleLabel);
    triangleLayout->addLayout(scaleLayout);
    
    mainLayout->addWidget(triangleGroup);
    
    // 添加通用控件（相机、光照等）
    mainLayout->addWidget(createCameraControls(panel));
    
    // 信息组
    QGroupBox *infoGroup = new QGroupBox("Information");
    QVBoxLayout *infoLayout = new QVBoxLayout(infoGroup);
    
    QLabel *infoLabel = new QLabel(
        "<b>Controls:</b><br>"
        "• WASD - Move camera<br>"
        "• Q/E - Move up/down<br>"
        "• Mouse drag - Rotate view<br>"
        "• Mouse wheel - Zoom<br>"
        "• R - Reset camera<br><br>"
        "<b>About:</b><br>"
        "This demo shows a simple colored<br>"
        "triangle with vertex color interpolation."
    );
    infoLabel->setWordWrap(true);
    infoLayout->addWidget(infoLabel);
    
    mainLayout->addWidget(infoGroup);
    
    // 添加弹性空间
    mainLayout->addStretch();
    
    return panel;
}

// ============================================
// 槽函数
// ============================================

void TriangleDemo::onRotationSpeedChanged(double value)
{
    rotationSpeed = static_cast<float>(value);
    emit statusMessage(QString("Rotation speed: %1 °/s").arg(value, 0, 'f', 1));
}

void TriangleDemo::onAutoRotateChanged(bool enabled)
{
    autoRotate = enabled;
    emit statusMessage(enabled ? "Auto rotation enabled" : "Auto rotation disabled");
}

void TriangleDemo::onResetRotation()
{
    rotation = 0.0f;
    emit statusMessage("Rotation reset");
    emit parameterChanged();
}