#include "CADDemo.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <glm/gtc/matrix_transform.hpp>

CADDemo::CADDemo(QObject *parent)
    : Demo(parent)
    , document_(std::make_unique<Document>())
    , renderer_(std::make_unique<Renderer>())
    , gridRenderer_(std::make_unique<GridRenderer>())
    , axisRenderer_(std::make_unique<AxisRenderer>())
    , showGrid_(true)
    , showAxis_(true)
    , documentDirty_(true)
    , isPanning_(false)
{
    // 设置 CAD 相机的默认位置（俯视图）
    camera->SetPosition(glm::vec3(0.0f, 0.0f, 10.0f));
    camera->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    camera->SetType(CameraType::ORBIT);
    
    // 初始化视口状态
    viewportState_.width = viewportWidth;
    viewportState_.height = viewportHeight;
}

CADDemo::~CADDemo()
{
    cleanup();
}

// ============================================
// Demo 基类接口实现
// ============================================

void CADDemo::initialize()
{
    // 初始化 Renderer（会自动调用 initializeOpenGLFunctions）
    if (!renderer_->initialize()) {
        emit statusMessage("Failed to initialize renderer");
        return;
    }
    
    // 添加测试实体
    addTestEntities();
    
    emit statusMessage("CAD Demo initialized");
}

void CADDemo::update(float deltaTime)
{
    // CAD 应用通常不需要每帧更新
    // 但可以在这里处理动画、相机平滑移动等
    
    if (documentDirty_) {
        syncRendererFromDocument();
        documentDirty_ = false;
    }
}

void CADDemo::render()
{
    // 更新视口状态
    updateViewportState();
    
    // 设置 OpenGL 状态（假设已经在外部有OpenGL上下文）
    // 注意：这些调用需要在有OpenGL上下文的地方进行
    // 如果Demo基类或GLWidget已经设置，这里可以省略
    
    // 绘制网格（最先绘制，在背景）
    if (showGrid_) {
        gridRenderer_->draw(*renderer_, viewportState_);
    }
    
    // 绘制坐标轴
    if (showAxis_) {
        axisRenderer_->draw(*renderer_, viewportState_, 100.0f);
    }
    
    // 绘制文档中的所有实体
    renderer_->draw(viewportState_);
}

void CADDemo::cleanup()
{
    if (renderer_) {
        renderer_->shutdown();
    }
    
    if (document_) {
        document_->clear();
    }
}

// ============================================
// 输入处理
// ============================================

void CADDemo::processKeyPress(CameraMovement qtKey, float deltaTime)
{
    // 调用基类的相机控制
    Demo::processKeyPress(qtKey, deltaTime);
    
    // CAD 特定的快捷键
    switch (qtKey) {
        case CameraMovement::RESET:
            resetView();
            break;
        default:
            break;
    }
}

void CADDemo::processMousePress(QPoint point)
{
    isPanning_ = true;
    lastMousePos_ = point;
}

void CADDemo::processMouseMove(QPoint pos)
{
    if (!isPanning_) return;
    
    QPoint delta = pos - lastMousePos_;
    lastMousePos_ = pos;
    
    // 根据相机类型处理鼠标移动
    if (camera->GetType() == CameraType::ORBIT) {
        // 轨道相机：旋转
        float xOffset = delta.x() * 0.5f;
        float yOffset = -delta.y() * 0.5f;
        camera->processMouseMovement(xOffset, yOffset);
    } else {
        // 其他相机类型：平移
        float sensitivity = viewportState_.worldPerPixel * 2.0f;
        glm::vec3 right = camera->getRight();
        glm::vec3 up = camera->getUp();
        
        glm::vec3 offset = -right * (float)delta.x() * sensitivity 
                         + up * (float)delta.y() * sensitivity;
        
        camera->position += offset;
        camera->target += offset;
    }
    
    emit parameterChanged();
}

void CADDemo::processMouseRelease()
{
    isPanning_ = false;
}

void CADDemo::processMouseWheel(int offset)
{
    // 缩放
    float delta = (float)offset / 120.0f;
    camera->processMouseScroll(delta);
    
    // 更新视口的 worldPerPixel（用于自适应细分）
    viewportState_.updateWorldPerPixel();
    documentDirty_ = true;  // 标记需要重新细分圆弧
    
    emit parameterChanged();
}

void CADDemo::resizeViewport(int width, int height)
{
    Demo::resizeViewport(width, height);
    
    viewportState_.width = width;
    viewportState_.height = height;
    viewportState_.updateWorldPerPixel();
    
    documentDirty_ = true;
}

// ============================================
// 公共槽函数
// ============================================

void CADDemo::setGridVisible(bool visible)
{
    showGrid_ = visible;
    emit parameterChanged();
}

void CADDemo::setAxisVisible(bool visible)
{
    showAxis_ = visible;
    emit parameterChanged();
}

void CADDemo::resetView()
{
    camera->SetPosition(glm::vec3(0.0f, 0.0f, 10.0f));
    camera->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    camera->reset();
    
    viewportState_.updateWorldPerPixel();
    documentDirty_ = true;
    
    emit statusMessage("View reset");
    emit parameterChanged();
}

void CADDemo::addTestEntities()
{
    // 添加一些测试图元
    
    // 红色正方形（Polyline）
    std::vector<glm::vec3> square = {
        {-2.0f, -2.0f, 0.0f},
        { 2.0f, -2.0f, 0.0f},
        { 2.0f,  2.0f, 0.0f},
        {-2.0f,  2.0f, 0.0f}
    };
    document_->addPolyline(square, true, Style::fromRGBA(255, 0, 0, 255));
    
    // 蓝色圆
    document_->addCircle(glm::vec3(0.0f, 0.0f, 0.0f), 1.5f, 
                        Style::fromRGBA(0, 0, 255, 255));
    
    // 绿色直线
    document_->addLine(glm::vec3(-3.0f, 0.0f, 0.0f), 
                      glm::vec3(3.0f, 0.0f, 0.0f),
                      Style::fromRGBA(0, 255, 0, 255));
    
    // 黄色圆弧（90度）
    document_->addArc(glm::vec3(2.0f, 2.0f, 0.0f), 1.0f, 
                     0.0f, glm::radians(90.0f),
                     Style::fromRGBA(255, 255, 0, 255));
    
    // 青色折线
    std::vector<glm::vec3> polyline = {
        {-3.0f, -3.0f, 0.0f},
        {-2.0f, -2.5f, 0.0f},
        {-1.0f, -3.0f, 0.0f},
        { 0.0f, -2.0f, 0.0f}
    };
    document_->addPolyline(polyline, false, Style::fromRGBA(0, 255, 255, 255));
    
    documentDirty_ = true;
    emit documentChanged();
    emit statusMessage(QString("Added 5 test entities"));
}

void CADDemo::clearDocument()
{
    document_->clear();
    documentDirty_ = true;
    
    emit documentChanged();
    emit statusMessage("Document cleared");
}

// ============================================
// 辅助函数
// ============================================

void CADDemo::updateViewportState()
{
    viewportState_.width = viewportWidth;
    viewportState_.height = viewportHeight;
    viewportState_.view = getViewMatrix();
    viewportState_.proj = getProjectionMatrix();
    viewportState_.updateWorldPerPixel();
}

void CADDemo::syncRendererFromDocument()
{
    // 同步文档到渲染器（增量更新）
    renderer_->syncFromDocument(*document_, viewportState_);
    
    // 清除文档的脏标记
    document_->clearAllDirtyFlags();
}

// ============================================
// 控制面板
// ============================================

QWidget* CADDemo::createControlPanel(QWidget *parent)
{
    QWidget *panel = new QWidget(parent);
    QVBoxLayout *layout = new QVBoxLayout(panel);
    
    // CAD 特定控件
    layout->addWidget(createCADControls(panel));
    layout->addWidget(createDocumentControls(panel));
    
    // 通用控件
    layout->addWidget(createCameraControls(panel));
    
    // 添加弹性空间
    layout->addStretch();
    
    return panel;
}

QWidget* CADDemo::createCADControls(QWidget *parent)
{
    QGroupBox *group = new QGroupBox("View Options", parent);
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    // 网格显示
    QCheckBox *gridCheckBox = new QCheckBox("Show Grid");
    gridCheckBox->setChecked(showGrid_);
    connect(gridCheckBox, &QCheckBox::toggled, this, &CADDemo::setGridVisible);
    layout->addWidget(gridCheckBox);
    
    // 坐标轴显示
    QCheckBox *axisCheckBox = new QCheckBox("Show Axis");
    axisCheckBox->setChecked(showAxis_);
    connect(axisCheckBox, &QCheckBox::toggled, this, &CADDemo::setAxisVisible);
    layout->addWidget(axisCheckBox);
    
    // 重置视图按钮
    QPushButton *resetViewBtn = new QPushButton("Reset View");
    connect(resetViewBtn, &QPushButton::clicked, this, &CADDemo::resetView);
    layout->addWidget(resetViewBtn);
    
    // 显示统计信息
    QLabel *statsLabel = new QLabel();
    auto updateStats = [this, statsLabel]() {
        int entityCount = document_->all().size();
        statsLabel->setText(QString("Entities: %1\nWorld/Pixel: %2")
            .arg(entityCount)
            .arg(viewportState_.worldPerPixel, 0, 'f', 4));
    };
    updateStats();
    connect(this, &CADDemo::documentChanged, updateStats);
    connect(this, &CADDemo::parameterChanged, updateStats);
    layout->addWidget(statsLabel);
    
    return group;
}

QWidget* CADDemo::createDocumentControls(QWidget *parent)
{
    QGroupBox *group = new QGroupBox("Document", parent);
    QVBoxLayout *layout = new QVBoxLayout(group);
    
    // 添加测试实体按钮
    QPushButton *addTestBtn = new QPushButton("Add Test Entities");
    connect(addTestBtn, &QPushButton::clicked, this, &CADDemo::addTestEntities);
    layout->addWidget(addTestBtn);
    
    // 清除文档按钮
    QPushButton *clearBtn = new QPushButton("Clear Document");
    connect(clearBtn, &QPushButton::clicked, this, &CADDemo::clearDocument);
    layout->addWidget(clearBtn);
    
    // TODO: 将来添加更多文档操作
    // - 保存/加载
    // - 添加特定图元
    // - 图层管理
    
    return group;
}