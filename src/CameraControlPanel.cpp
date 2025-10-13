#include "CameraControlPanel.h"
#include <QFormLayout>
#include <QHBoxLayout>

CameraControlPanel::CameraControlPanel(QWidget *parent)
    : QWidget(parent)
    , camera(nullptr)
    , isUpdating(false)
{
    setupUI();
}

void CameraControlPanel::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(10);
    
    // ============================================
    // 相机类型选择
    // ============================================
    QFormLayout *typeLayout = new QFormLayout();
    
    cameraTypeCombo = new QComboBox();
    cameraTypeCombo->addItem("Orbit");
    cameraTypeCombo->addItem("FPS");
    cameraTypeCombo->addItem("Free");
    connect(cameraTypeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CameraControlPanel::onCameraTypeChanged);
    
    typeLayout->addRow("Camera Type:", cameraTypeCombo);
    mainLayout->addLayout(typeLayout);
    
    // ============================================
    // Orbit 相机控件组（仅在 Orbit 模式显示）
    // ============================================
    orbitGroup = new QGroupBox("Orbit Settings");
    QFormLayout *orbitLayout = new QFormLayout(orbitGroup);
    
    // Orbit Radius
    orbitRadiusSpin = new QDoubleSpinBox();
    orbitRadiusSpin->setRange(1.0, 50.0);
    orbitRadiusSpin->setSingleStep(0.5);
    orbitRadiusSpin->setDecimals(2);
    orbitRadiusSpin->setSuffix(" units");
    connect(orbitRadiusSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CameraControlPanel::onOrbitRadiusChanged);
    orbitLayout->addRow("Radius:", orbitRadiusSpin);
    
    // Orbit Yaw
    orbitYawSpin = new QDoubleSpinBox();
    orbitYawSpin->setRange(-180.0, 180.0);
    orbitYawSpin->setSingleStep(5.0);
    orbitYawSpin->setDecimals(1);
    orbitYawSpin->setSuffix("°");
    connect(orbitYawSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CameraControlPanel::onOrbitYawChanged);
    orbitLayout->addRow("Yaw:", orbitYawSpin);
    
    // Orbit Pitch
    orbitPitchSpin = new QDoubleSpinBox();
    orbitPitchSpin->setRange(-89.0, 89.0);
    orbitPitchSpin->setSingleStep(5.0);
    orbitPitchSpin->setDecimals(1);
    orbitPitchSpin->setSuffix("°");
    connect(orbitPitchSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CameraControlPanel::onOrbitPitchChanged);
    orbitLayout->addRow("Pitch:", orbitPitchSpin);
    
    // Target Position (X, Y, Z)
    QLabel *targetLabel = new QLabel("Target:");
    QHBoxLayout *targetLayout = new QHBoxLayout();
    
    targetXSpin = new QDoubleSpinBox();
    targetXSpin->setRange(-10.0, 10.0);
    targetXSpin->setPrefix("X: ");
    targetXSpin->setDecimals(2);
    connect(targetXSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CameraControlPanel::onTargetXChanged);
    
    targetYSpin = new QDoubleSpinBox();
    targetYSpin->setRange(-10.0, 10.0);
    targetYSpin->setPrefix("Y: ");
    targetYSpin->setDecimals(2);
    connect(targetYSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CameraControlPanel::onTargetYChanged);
    
    targetZSpin = new QDoubleSpinBox();
    targetZSpin->setRange(-10.0, 10.0);
    targetZSpin->setPrefix("Z: ");
    targetZSpin->setDecimals(2);
    connect(targetZSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CameraControlPanel::onTargetZChanged);
    
    targetLayout->addWidget(targetXSpin);
    targetLayout->addWidget(targetYSpin);
    targetLayout->addWidget(targetZSpin);
    
    orbitLayout->addRow(targetLabel, targetLayout);
    
    mainLayout->addWidget(orbitGroup);
    
    // ============================================
    // 通用相机控件
    // ============================================
    QGroupBox *generalGroup = new QGroupBox("General Settings");
    QFormLayout *generalLayout = new QFormLayout(generalGroup);
    
    // FOV
    fovSpin = new QDoubleSpinBox();
    fovSpin->setRange(1.0, 90.0);
    fovSpin->setSingleStep(1.0);
    fovSpin->setDecimals(1);
    fovSpin->setSuffix("°");
    fovSpin->setValue(45.0);
    connect(fovSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CameraControlPanel::onFovChanged);
    generalLayout->addRow("FOV:", fovSpin);
    
    // Move Speed
    moveSpeedSpin = new QDoubleSpinBox();
    moveSpeedSpin->setRange(0.1, 10.0);
    moveSpeedSpin->setSingleStep(0.1);
    moveSpeedSpin->setDecimals(2);
    moveSpeedSpin->setValue(2.5);
    connect(moveSpeedSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CameraControlPanel::onMoveSpeedChanged);
    generalLayout->addRow("Move Speed:", moveSpeedSpin);
    
    // Mouse Sensitivity
    mouseSensitivitySpin = new QDoubleSpinBox();
    mouseSensitivitySpin->setRange(10.0, 500.0);
    mouseSensitivitySpin->setSingleStep(10.0);
    mouseSensitivitySpin->setDecimals(1);
    mouseSensitivitySpin->setValue(100.0);
    connect(mouseSensitivitySpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
            this, &CameraControlPanel::onMouseSensitivityChanged);
    generalLayout->addRow("Mouse Sensitivity:", mouseSensitivitySpin);
    
    mainLayout->addWidget(generalGroup);
    
    // 添加弹性空间
    mainLayout->addStretch();
    
    // 初始状态：隐藏 Orbit 控件
    updateOrbitControlsVisibility();
}

void CameraControlPanel::setCamera(Camera *cam)
{
    camera = cam;
    updateFromCamera();
}

void CameraControlPanel::updateFromCamera()
{
    if (!camera) return;
    
    isUpdating = true;
    
    // 更新相机类型
    cameraTypeCombo->setCurrentIndex((int)camera->GetType());
    
    // 更新 Orbit 参数
    if (camera->type == CameraType::ORBIT) {
        orbitRadiusSpin->setValue(camera->radius);
        orbitYawSpin->setValue(camera->yaw);
        orbitPitchSpin->setValue(camera->pitch);
        targetXSpin->setValue(camera->target.x);
        targetYSpin->setValue(camera->target.y);
        targetZSpin->setValue(camera->target.z);
    }
    
    // 更新通用参数
    fovSpin->setValue(camera->fov);
    moveSpeedSpin->setValue(camera->moveSpeed);
    mouseSensitivitySpin->setValue(camera->mouseSensitivity);
    
    updateOrbitControlsVisibility();
    
    isUpdating = false;
}

void CameraControlPanel::updateOrbitControlsVisibility()
{
    bool isOrbit = (cameraTypeCombo->currentIndex() == (int)CameraType::ORBIT);
    orbitGroup->setVisible(isOrbit);
}

// ============================================
// 槽函数 - 当控件值改变时触发
// ============================================

void CameraControlPanel::onCameraTypeChanged(int index)
{
    if (isUpdating) return;
    
    CameraType type = (CameraType)index;
    emit cameraTypeChanged(type);
    
    if (camera) {
        camera->SetType(type);
    }
    
    updateOrbitControlsVisibility();
}

void CameraControlPanel::onOrbitRadiusChanged(double value)
{
    if (isUpdating) return;
    
    emit orbitRadiusChanged((float)value);
    
    if (camera) {
        camera->radius = (float)value;
    }
}

void CameraControlPanel::onOrbitYawChanged(double value)
{
    if (isUpdating) return;
    
    emit orbitYawChanged((float)value);
    
    if (camera) {
        camera->yaw = (float)value;
    }
}

void CameraControlPanel::onOrbitPitchChanged(double value)
{
    if (isUpdating) return;
    
    emit orbitPitchChanged((float)value);
    
    if (camera) {
        camera->pitch = (float)value;
    }
}

void CameraControlPanel::onTargetXChanged(double value)
{
    if (isUpdating) return;
    
    if (camera) {
        camera->target.x = (float)value;
        emit targetChanged(camera->target.x, camera->target.y, camera->target.z);
    }
}

void CameraControlPanel::onTargetYChanged(double value)
{
    if (isUpdating) return;
    
    if (camera) {
        camera->target.y = (float)value;
        emit targetChanged(camera->target.x, camera->target.y, camera->target.z);
    }
}

void CameraControlPanel::onTargetZChanged(double value)
{
    if (isUpdating) return;
    
    if (camera) {
        camera->target.z = (float)value;
        emit targetChanged(camera->target.x, camera->target.y, camera->target.z);
    }
}

void CameraControlPanel::onFovChanged(double value)
{
    if (isUpdating) return;
    
    emit fovChanged((float)value);
    
    if (camera) {
        camera->fov = (float)value;
    }
}

void CameraControlPanel::onMoveSpeedChanged(double value)
{
    if (isUpdating) return;
    
    emit moveSpeedChanged((float)value);
    
    if (camera) {
        camera->moveSpeed = (float)value;
    }
}

void CameraControlPanel::onMouseSensitivityChanged(double value)
{
    if (isUpdating) return;
    
    emit mouseSensitivityChanged((float)value);
    
    if (camera) {
        camera->mouseSensitivity = (float)value;
    }
}