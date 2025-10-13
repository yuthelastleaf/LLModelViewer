#ifndef CAMERACONTROLPANEL_H
#define CAMERACONTROLPANEL_H

#include <QWidget>
#include <QComboBox>
#include <QSlider>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QVBoxLayout>
#include <QGroupBox>

// 假设你有一个 Camera 类
enum class CameraType {
    ORBIT = 0,
    FPS = 1,
    FREE = 2
};

class Camera; // 前向声明

class CameraControlPanel : public QWidget
{
    Q_OBJECT

public:
    explicit CameraControlPanel(QWidget *parent = nullptr);
    
    // 设置要控制的相机
    void setCamera(Camera *cam);
    
    // 从相机更新 UI（当相机从外部改变时调用）
    void updateFromCamera();

signals:
    // 当参数改变时发射信号
    void cameraTypeChanged(CameraType type);
    void orbitRadiusChanged(float radius);
    void orbitYawChanged(float yaw);
    void orbitPitchChanged(float pitch);
    void targetChanged(float x, float y, float z);
    void fovChanged(float fov);
    void moveSpeedChanged(float speed);
    void mouseSensitivityChanged(float sensitivity);

private slots:
    void onCameraTypeChanged(int index);
    void onOrbitRadiusChanged(double value);
    void onOrbitYawChanged(double value);
    void onOrbitPitchChanged(double value);
    void onTargetXChanged(double value);
    void onTargetYChanged(double value);
    void onTargetZChanged(double value);
    void onFovChanged(double value);
    void onMoveSpeedChanged(double value);
    void onMouseSensitivityChanged(double value);

private:
    void setupUI();
    void updateOrbitControlsVisibility();
    
    Camera *camera;
    
    // 控件
    QComboBox *cameraTypeCombo;
    
    // Orbit 相关控件
    QGroupBox *orbitGroup;
    QDoubleSpinBox *orbitRadiusSpin;
    QDoubleSpinBox *orbitYawSpin;
    QDoubleSpinBox *orbitPitchSpin;
    QDoubleSpinBox *targetXSpin;
    QDoubleSpinBox *targetYSpin;
    QDoubleSpinBox *targetZSpin;
    
    // 通用控件
    QDoubleSpinBox *fovSpin;
    QDoubleSpinBox *moveSpeedSpin;
    QDoubleSpinBox *mouseSensitivitySpin;
    
    // 是否正在更新（避免循环触发）
    bool isUpdating;
};

#endif // CAMERACONTROLPANEL_H