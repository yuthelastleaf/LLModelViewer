#include "TriangleDemo.h"
#include <glad/glad.h>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QColorDialog>

// 顶点着色器
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}
)";

// 片段着色器
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

uniform vec3 color;

void main()
{
    FragColor = vec4(color, 1.0);
}
)";

TriangleDemo::TriangleDemo(QObject *parent)
    : Demo(parent)
    , shaderProgram(0)
    , VAO(0)
    , VBO(0)
    , triangleColor(1.0f, 0.5f, 0.2f)
    , rotationAngle(0.0f)
    , rotationSpeed(45.0f)
    , autoRotate(true)
{
}

TriangleDemo::~TriangleDemo()
{
    cleanup();
}

void TriangleDemo::initialize()
{
    setupShaders();
    setupGeometry();
    
    emit statusMessage("Triangle Demo initialized");
}

void TriangleDemo::setupShaders()
{
    // 编译顶点着色器
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);
    
    // 检查编译错误
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        emit statusMessage(QString("Vertex shader compilation failed: %1").arg(infoLog));
    }
    
    // 编译片段着色器
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        emit statusMessage(QString("Fragment shader compilation failed: %1").arg(infoLog));
    }
    
    // 链接着色器程序
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
        emit statusMessage(QString("Shader program linking failed: %1").arg(infoLog));
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void TriangleDemo::setupGeometry()
{
    // 三角形顶点数据
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };
    
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void TriangleDemo::update(float deltaTime)
{
    if (autoRotate) {
        rotationAngle += rotationSpeed * deltaTime;
        if (rotationAngle > 360.0f) {
            rotationAngle -= 360.0f;
        }
    }
}

void TriangleDemo::render()
{
    // 渲染网格和坐标轴（如果有）
    if (gridAxisHelper) {
        gridAxisHelper->render(getViewMatrix(), getProjectionMatrix());
    }
    
    // 使用着色器程序
    glUseProgram(shaderProgram);
    
    // 设置变换矩阵
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::rotate(model, glm::radians(rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f));
    
    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int viewLoc = glGetUniformLocation(shaderProgram, "view");
    int projectionLoc = glGetUniformLocation(shaderProgram, "projection");
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &getViewMatrix()[0][0]);
    glUniformMatrix4fv(projectionLoc, 1, GL_FALSE, &getProjectionMatrix()[0][0]);
    
    // 设置颜色
    int colorLoc = glGetUniformLocation(shaderProgram, "color");
    glUniform3fv(colorLoc, 1, &triangleColor[0]);
    
    // 绘制三角形
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
}

void TriangleDemo::cleanup()
{
    if (VAO) {
        glDeleteVertexArrays(1, &VAO);
        VAO = 0;
    }
    if (VBO) {
        glDeleteBuffers(1, &VBO);
        VBO = 0;
    }
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
}

QWidget* TriangleDemo::createControlPanel(QWidget *parent)
{
    QWidget *panel = new QWidget(parent);
    QVBoxLayout *mainLayout = new QVBoxLayout(panel);
    
    // ============================================
    // 三角形控制组
    // ============================================
    QGroupBox *triangleGroup = new QGroupBox("Triangle Controls");
    QVBoxLayout *triangleLayout = new QVBoxLayout(triangleGroup);
    
    // 颜色选择按钮
    QPushButton *colorBtn = new QPushButton("Choose Color");
    connect(colorBtn, &QPushButton::clicked, [this]() {
        QColor currentColor(
            (int)(triangleColor.r * 255),
            (int)(triangleColor.g * 255),
            (int)(triangleColor.b * 255)
        );
        
        QColor newColor = QColorDialog::getColor(currentColor, nullptr, "Choose Triangle Color");
        if (newColor.isValid()) {
            triangleColor.r = newColor.redF();
            triangleColor.g = newColor.greenF();
            triangleColor.b = newColor.blueF();
            emit parameterChanged();
        }
    });
    triangleLayout->addWidget(colorBtn);
    
    // 自动旋转开关
    QCheckBox *autoRotateCheck = new QCheckBox("Auto Rotate");
    autoRotateCheck->setChecked(autoRotate);
    connect(autoRotateCheck, &QCheckBox::toggled, [this](bool checked) {
        autoRotate = checked;
        emit statusMessage(checked ? "Auto rotation enabled" : "Auto rotation disabled");
    });
    triangleLayout->addWidget(autoRotateCheck);
    
    // 旋转速度滑块
    QLabel *speedLabel = new QLabel("Rotation Speed:");
    triangleLayout->addWidget(speedLabel);
    
    QSlider *speedSlider = new QSlider(Qt::Horizontal);
    speedSlider->setRange(0, 180);
    speedSlider->setValue((int)rotationSpeed);
    speedSlider->setTickPosition(QSlider::TicksBelow);
    speedSlider->setTickInterval(30);
    
    QLabel *speedValueLabel = new QLabel(QString::number((int)rotationSpeed) + "°/s");
    
    connect(speedSlider, &QSlider::valueChanged, [this, speedValueLabel](int value) {
        rotationSpeed = (float)value;
        speedValueLabel->setText(QString::number(value) + "°/s");
    });
    
    triangleLayout->addWidget(speedSlider);
    triangleLayout->addWidget(speedValueLabel);
    
    // 重置按钮
    QPushButton *resetBtn = new QPushButton("Reset Rotation");
    connect(resetBtn, &QPushButton::clicked, [this]() {
        rotationAngle = 0.0f;
        emit statusMessage("Rotation reset");
    });
    triangleLayout->addWidget(resetBtn);
    
    mainLayout->addWidget(triangleGroup);
    
    // ============================================
    // 添加基类的通用控件
    // ============================================
    mainLayout->addWidget(createCameraControls(panel));
    mainLayout->addWidget(createGridAxisControls(panel));
    
    mainLayout->addStretch();
    
    return panel;
}