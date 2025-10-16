#include "Demo.h"
#include "light/LightManager.h"
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QCheckBox>
#include <QComboBox>
#include <QDoubleSpinBox>

Demo::Demo(QObject *parent)
    : QObject(parent)
    , camera(std::make_unique<Camera>())
    , lightManager(std::make_unique<LightManager>())
    , viewportWidth(800)
    , viewportHeight(600)
{
    // 设置默认相机
    camera->SetPosition(glm::vec3(0.0f, 0.0f, 5.0f));
    camera->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    
    // 创建默认光源
    auto dirLight = lightManager->createDirectionalLight();
    dirLight->direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    dirLight->ambient = glm::vec3(0.2f);
    dirLight->diffuse = glm::vec3(0.5f);
    dirLight->specular = glm::vec3(1.0f);
}

Demo::~Demo()
{
    // cleanup() 应该在子类中调用
}

// ============================================
// 默认输入处理实现
// ============================================

void Demo::processKeyPress(CameraMovement qtKey, float deltaTime)
{    
    switch (qtKey) {
        case CameraMovement::FORWARD:
        case CameraMovement::BACKWARD:
        case CameraMovement::LEFT:
        case CameraMovement::RIGHT:
        case CameraMovement::UP:
        case CameraMovement::DOWN:
            camera->processKeyboard(qtKey, deltaTime);
            break;
        case CameraMovement::RESET:
            camera->reset();
            emit statusMessage("Camera reset");
            emit parameterChanged();
            break;
        default:
            break;
    }
}

void Demo::processMousePress(QPoint point)
{

}

void Demo::processMouseMove(QPoint pos)
{
    float xOffset = pos.x() * 0.1f;
    float yOffset = -pos.y() * 0.1f; // 反转 Y 轴
    camera->processMouseMovement(xOffset, yOffset);
    emit parameterChanged();
}

void Demo::processMouseRelease()
{
    
}

void Demo::processMouseWheel(int offset)
{
    float yOffset = (float)offset / 120.0f;
    camera->processMouseScroll(yOffset);
    emit parameterChanged();
}

void Demo::resizeViewport(int width, int height)
{
    viewportWidth = width;
    viewportHeight = height;
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
    
    // 添加弹性空间
    layout->addStretch();
    
    return panel;
}

QWidget* Demo::createCameraControls(QWidget *parent)
{
    QGroupBox *group = new QGroupBox("Camera Controls", parent);
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    // 相机类型选择
    QHBoxLayout *typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("Type:"));
    QComboBox *typeCombo = new QComboBox();
    typeCombo->addItem("Orbit", static_cast<int>(CameraType::ORBIT));
    typeCombo->addItem("FPS", static_cast<int>(CameraType::FPS));
    typeCombo->addItem("Free", static_cast<int>(CameraType::FREE));
    typeCombo->setCurrentIndex(0);
    connect(typeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this, typeCombo](int index) {
        CameraType type = static_cast<CameraType>(typeCombo->currentData().toInt());
        camera->SetType(type);
        emit statusMessage(QString("Camera type changed to: %1").arg(typeCombo->currentText()));
        emit parameterChanged();
    });
    typeLayout->addWidget(typeCombo);
    layout->addLayout(typeLayout);
    
    // FOV 控制
    QHBoxLayout *fovLayout = new QHBoxLayout();
    fovLayout->addWidget(new QLabel("FOV:"));
    QSlider *fovSlider = new QSlider(Qt::Horizontal);
    fovSlider->setRange(10, 90);
    fovSlider->setValue(static_cast<int>(camera->getFov()));
    QLabel *fovLabel = new QLabel(QString::number(camera->getFov(), 'f', 0));
    connect(fovSlider, &QSlider::valueChanged, [this, fovLabel](int value) {
        camera->fov = static_cast<float>(value);
        fovLabel->setText(QString::number(value));
        emit parameterChanged();
    });
    fovLayout->addWidget(fovSlider);
    fovLayout->addWidget(fovLabel);
    layout->addLayout(fovLayout);
    
    // 移动速度控制
    QHBoxLayout *speedLayout = new QHBoxLayout();
    speedLayout->addWidget(new QLabel("Move Speed:"));
    QDoubleSpinBox *speedSpinBox = new QDoubleSpinBox();
    speedSpinBox->setRange(0.1, 10.0);
    speedSpinBox->setSingleStep(0.5);
    speedSpinBox->setValue(camera->moveSpeed);
    connect(speedSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), [this](double value) {
        camera->moveSpeed = static_cast<float>(value);
    });
    speedLayout->addWidget(speedSpinBox);
    layout->addLayout(speedLayout);
    
    // 重置按钮
    QPushButton *resetBtn = new QPushButton("Reset Camera");
    connect(resetBtn, &QPushButton::clicked, [this]() {
        camera->reset();
        emit statusMessage("Camera reset");
        emit parameterChanged();
    });
    layout->addWidget(resetBtn);
    
    // 控制说明
    QLabel *helpLabel = new QLabel(
        "Controls:\n"
        "• WASD - Move forward/left/back/right\n"
        "• Q/E - Move up/down\n"
        "• Mouse drag - Rotate view\n"
        "• Mouse wheel - Zoom\n"
        "• R - Reset camera"
    );
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("QLabel { color: gray; font-size: 9pt; }");
    layout->addWidget(helpLabel);
    
    return group;
}

QWidget* Demo::createLightControls(QWidget *parent)
{
    QGroupBox *group = new QGroupBox("Light Controls", parent);
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    const auto& lights = lightManager->getLights();
    
    if (lights.empty()) {
        layout->addWidget(new QLabel("No lights in scene"));
        
        QPushButton *addDirLightBtn = new QPushButton("Add Directional Light");
        connect(addDirLightBtn, &QPushButton::clicked, [this]() {
            lightManager->createDirectionalLight();
            emit statusMessage("Directional light added");
            emit parameterChanged();
        });
        layout->addWidget(addDirLightBtn);
    } else {
        // 显示现有光源信息
        for (size_t i = 0; i < lights.size(); ++i) {
            auto light = lights[i];
            
            QString lightName;
            switch (light->type) {
                case LightType::DIRECTIONAL:
                    lightName = QString("Directional Light %1").arg(i + 1);
                    break;
                case LightType::POINT:
                    lightName = QString("Point Light %1").arg(i + 1);
                    break;
                case LightType::SPOT:
                    lightName = QString("Spot Light %1").arg(i + 1);
                    break;
            }
            
            QCheckBox *enableCheckBox = new QCheckBox(lightName);
            enableCheckBox->setChecked(light->enabled);
            connect(enableCheckBox, &QCheckBox::toggled, [light](bool checked) {
                light->enabled = checked;
            });
            layout->addWidget(enableCheckBox);
        }
        
        // 添加光源按钮
        QHBoxLayout *addLayout = new QHBoxLayout();
        QPushButton *addDirBtn = new QPushButton("+ Dir");
        QPushButton *addPointBtn = new QPushButton("+ Point");
        QPushButton *addSpotBtn = new QPushButton("+ Spot");
        
        connect(addDirBtn, &QPushButton::clicked, [this]() {
            lightManager->createDirectionalLight();
            emit statusMessage("Directional light added");
            emit parameterChanged();
        });
        connect(addPointBtn, &QPushButton::clicked, [this]() {
            lightManager->createPointLight(glm::vec3(0.0f, 2.0f, 0.0f));
            emit statusMessage("Point light added");
            emit parameterChanged();
        });
        connect(addSpotBtn, &QPushButton::clicked, [this]() {
            lightManager->createSpotLight(glm::vec3(0.0f, 2.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f));
            emit statusMessage("Spot light added");
            emit parameterChanged();
        });
        
        addLayout->addWidget(addDirBtn);
        addLayout->addWidget(addPointBtn);
        addLayout->addWidget(addSpotBtn);
        layout->addLayout(addLayout);
        
        // 清除所有光源按钮
        QPushButton *clearBtn = new QPushButton("Clear All Lights");
        connect(clearBtn, &QPushButton::clicked, [this]() {
            lightManager->clear();
            emit statusMessage("All lights removed");
            emit parameterChanged();
        });
        layout->addWidget(clearBtn);
    }
    
    return group;
}