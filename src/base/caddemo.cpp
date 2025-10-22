#include "caddemo.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QPointer>
#include <glm/gtc/matrix_transform.hpp>

CADDemo::CADDemo(QObject *parent)
    : Demo(parent), document_(std::make_unique<Document>()), renderer_(std::make_unique<Renderer>()), gridRenderer_(std::make_unique<GridRenderer>()), axisRenderer_(std::make_unique<AxisRenderer>()), showGrid_(true), showAxis_(true), documentDirty_(true), isPanning_(false), cad_mode_(DrawMode::SELECT), cur_draw_(0)
{
    // ✅ 默认设置为 2D CAD 俯视图
    camera->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    // camera->SetTopView(10.0f);
    camera->SetTopView(10.0f);

    qDebug() << "CAD Camera initialized (2D mode):";
    qDebug() << "  Position:" << camera->position.x << camera->position.y << camera->position.z;
    qDebug() << "  Target:" << camera->target.x << camera->target.y << camera->target.z;
    qDebug() << "  Is 2D:" << camera->is2D();

    // 初始化视口状态
    viewportState_.width = viewportWidth;
    viewportState_.height = viewportHeight;

    // 默认 XY 平面
    workPlane_->setXY();

    // 启用跟随模式
    WorkPlane::FollowMode mode;
    mode.enabled = true;
    mode.followPosition = true;    // 原点跟随相机目标
    mode.followOrientation = true; // 法向量垂直于视线
    workPlane_->setFollowMode(mode);
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
    if (!renderer_->initialize())
    {
        emit statusMessage("Failed to initialize renderer");
        return;
    }

    addTestEntities();
    emit statusMessage("CAD Demo initialized");
}

void CADDemo::update(float deltaTime)
{
    if (documentDirty_)
    {
        syncRendererFromDocument();
        documentDirty_ = false;
    }
}

void CADDemo::render()
{
    updateViewportState();

    if (documentDirty_ || renderer_)
    {
        renderer_->syncFromDocument(*document_, viewportState_, false);
        documentDirty_ = false;
    }

    // 绘制网格
    if (showGrid_)
    {
        // 深色主题配色
        std::uint32_t minorColor = 0x40404040; // RGBA: (64, 64, 64, 64) - 深灰色，25%透明
        std::uint32_t majorColor = 0x80808080; // RGBA: (128, 128, 128, 128) - 灰色，50%透明
        gridRenderer_->draw(*renderer_, viewportState_, minorColor, majorColor, 5);
    }

    // 绘制坐标轴
    if (showAxis_)
    {
        bool drawZ = !camera->is2D();
        axisRenderer_->draw(*renderer_, viewportState_, 100.0f, 0xFF0000FF, // X - 红色
                            0x00FF00FF,                                     // Y - 绿色
                            0x0000FFFF,                                     // Z - 蓝色
                            drawZ);
    }

    // 绘制文档实体
    renderer_->draw(viewportState_);
}

void CADDemo::cleanup()
{
    if (renderer_)
    {
        renderer_->shutdown();
    }

    if (document_)
    {
        document_->clear();
    }
}

// ============================================
// 输入处理
// ============================================

void CADDemo::processKeyPress(CameraMovement qtKey, float deltaTime)
{
    Demo::processKeyPress(qtKey, deltaTime);

    if (qtKey == CameraMovement::RESET)
    {
        resetView();
    }
}

void CADDemo::processMousePress(QPoint point, glm::vec3 wpoint)
{
    if (isPanning_)
    {
        return;
    }
    isPanning_ = true;
    switch (cad_mode_)
    {
    case DrawMode::SELECT:
        break;

    case DrawMode::LINE:
    {
        qDebug() << "=== LINE Mode - Mouse Press ===";
        qDebug() << "World position:" << point.x() << point.y();
        qDebug() << "Start point:" << wpoint.x << wpoint.y << wpoint.z;

        cur_draw_ = document_->addLine(wpoint, wpoint,
                                       Style::fromRGBA(0, 255, 0, 255));
        qDebug() << "Created entity ID:" << cur_draw_;
        // 验证实体是否成功创建
        const Entity *entity = document_->get(cur_draw_);
        if (entity)
        {
            qDebug() << "Entity created successfully!";
            // qDebug() << "  Type:" << (int)entity->type;
            qDebug() << "  Visible:" << entity->visible;

            if (auto *line = std::get_if<Line>(&entity->geom))
            {
                qDebug() << "  Line p0:" << line->p0.x << line->p0.y << line->p0.z;
                qDebug() << "  Line p1:" << line->p1.x << line->p1.y << line->p1.z;
            }
        }
        else
        {
            qDebug() << "❌ Entity creation failed!";
        }

        emit documentChanged();
        qDebug() << "=== End LINE Mode ===\n";
        break;
    }

    case DrawMode::CIRCLE:
        emit statusMessage("Circle tool selected - Click to set center");
        qDebug() << "Switched to: Circle";
        break;

    case DrawMode::RECT:
        emit statusMessage("Rectangle tool selected - Click to set first corner");
        qDebug() << "Switched to: Rectangle";
        break;
    case DrawMode::BOX:
    {
        glm::vec3 centerPos;

        if (camera->is2D())
        {
            break;
        }

        Style boxStyle = Style::fromRGBA(100, 149, 237, 255);
        EntityId boxId = document_->addBox(centerPos, 1.0f, boxStyle);

        if (boxId != 0)
        {
            renderer_->syncFromDocument(*document_, viewportState_, false);
            documentDirty_ = true;

            emit statusMessage(QString("Box created at (%.2f, %.2f, %.2f)")
                                   .arg(centerPos.x)
                                   .arg(centerPos.y)
                                   .arg(centerPos.z));
            emit documentChanged();
        }
    }
    break;
    }
}

void CADDemo::processMouseMove(QPoint point, QPoint delta_point, glm::vec3 wpoint, glm::vec3 delta_wpoint)
{
    if (!isPanning_)
    {
        return;
    }

    switch (cad_mode_)
    {
    case DrawMode::SELECT:
        if (camera->is2D())
        {
            // ✅ 2D 模式：平移
            camera->pan2D((float)delta_point.x(), (float)delta_point.y(), viewportState_.worldPerPixel);
        }
        else
        {
            // ✅ 3D 模式：旋转
            float xOffset = delta_point.x() * 0.5f;
            float yOffset = -delta_point.y() * 0.5f;
            camera->processMouseMovement(xOffset, yOffset);
        }
        emit parameterChanged();
        break;

    case DrawMode::LINE:
        if (camera->is2D())
        {
            document_->updateEndLinePoint(cur_draw_, delta_wpoint);
            // qDebug() << "linepos :" << delta_wpoint.x << " " << delta_wpoint.y << " " << delta_wpoint.z;

            emit documentChanged();
        }
        break;

    case DrawMode::CIRCLE:
        emit statusMessage("Circle tool selected - Click to set center");
        qDebug() << "Switched to: Circle";
        break;

    case DrawMode::RECT:
        emit statusMessage("Rectangle tool selected - Click to set first corner");
        qDebug() << "Switched to: Rectangle";
        break;
    }
}

void CADDemo::processMouseRelease()
{
    qDebug() << "processMouseRelease called";
    isPanning_ = false;
}

void CADDemo::processMouseWheel(int offset)
{
    float delta = (float)offset / 120.0f;
    camera->processMouseScroll(delta);

    // 更新视口参数
    viewportState_.updateWorldPerPixel();
    documentDirty_ = true;

    emit parameterChanged();
}

void CADDemo::resizeViewport(int width, int height)
{
    Demo::resizeViewport(width, height);
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
    camera->reset();
    viewportState_.updateWorldPerPixel();
    documentDirty_ = true;

    emit statusMessage("View reset");
    emit parameterChanged();
}

void CADDemo::switch2DMode(bool enable)
{
    camera->set2DMode(enable);

    viewportState_.updateWorldPerPixel();
    documentDirty_ = true;

    emit statusMessage(enable ? "Switched to 2D mode" : "Switched to 3D mode");
    emit parameterChanged();
}

void CADDemo::setViewOrientation(int orientation)
{
    View2DOrientation view = static_cast<View2DOrientation>(orientation);

    switch (view)
    {
    case View2DOrientation::TOP:
        camera->SetTopView(camera->radius);
        emit statusMessage("Top view");
        break;
    case View2DOrientation::FRONT:
        camera->SetFrontView(camera->radius);
        emit statusMessage("Front view");
        break;
    case View2DOrientation::RIGHT:
        camera->SetRightView(camera->radius);
        emit statusMessage("Right view");
        break;
    }

    viewportState_.updateWorldPerPixel();
    documentDirty_ = true;
    emit parameterChanged();
}

void CADDemo::setIsometricView()
{
    camera->SetIsometricView(camera->radius);

    viewportState_.updateWorldPerPixel();
    documentDirty_ = true;

    emit statusMessage("Isometric view");
    emit parameterChanged();
}

void CADDemo::addTestEntities()
{
    // 红色正方形
    std::vector<glm::vec3> square = {
        {-2.0f, -2.0f, 0.0f},
        {2.0f, -2.0f, 0.0f},
        {2.0f, 2.0f, 0.0f},
        {-2.0f, 2.0f, 0.0f}};
    document_->addPolyline(square, true, Style::fromRGBA(255, 0, 0, 255));

    // 蓝色圆
    document_->addCircle(glm::vec3(0.0f, 0.0f, 0.0f), 1.5f,
                         Style::fromRGBA(0, 0, 255, 255));

    // 绿色直线
    document_->addLine(glm::vec3(-3.0f, 0.0f, 0.0f),
                       glm::vec3(3.0f, 0.0f, 0.0f),
                       Style::fromRGBA(0, 255, 0, 255));

    // 黄色圆弧
    document_->addArc(glm::vec3(2.0f, 2.0f, 0.0f), 1.0f,
                      0.0f, glm::radians(90.0f),
                      Style::fromRGBA(255, 255, 0, 255));

    // 青色折线
    std::vector<glm::vec3> polyline = {
        {-3.0f, -3.0f, 0.0f},
        {-2.0f, -2.5f, 0.0f},
        {-1.0f, -3.0f, 0.0f},
        {0.0f, -2.0f, 0.0f}};
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

void CADDemo::onDrawModeChanged(int id)
{
    cad_mode_ = static_cast<DrawMode>(id);

    switch (cad_mode_)
    {
    case DrawMode::SELECT:
        emit statusMessage("Selection tool active");
        qDebug() << "Switched to: Select";
        break;

    case DrawMode::LINE:
        emit statusMessage("Line tool selected - Click to set start point");
        qDebug() << "Switched to: Line";
        break;

    case DrawMode::CIRCLE:
        emit statusMessage("Circle tool selected - Click to set center");
        qDebug() << "Switched to: Circle";
        break;

    case DrawMode::RECT:
        emit statusMessage("Rectangle tool selected - Click to set first corner");
        qDebug() << "Switched to: Rectangle";
        break;
    }
}

// ============================================
// 辅助函数
// ============================================

// ✅ 重写基类的 updateViewportState
void CADDemo::updateViewportState()
{
    // 先调用基类实现更新基本信息
    Demo::updateViewportState();

    // ✅ 更新工作平面跟随
    if (workPlane_ && workPlane_->getFollowMode().enabled)
    {
        workPlane_->updateFollow(
            camera->getPosition(),
            camera->getFront(),
            camera->getTarget());
    }
}

void CADDemo::syncRendererFromDocument()
{
    renderer_->syncFromDocument(*document_, viewportState_);
    document_->clearAllDirtyFlags();
}

// ============================================
// 控制面板
// ============================================

QWidget *CADDemo::createControlPanel(QWidget *parent)
{
    QWidget *panel = new QWidget(parent);
    QVBoxLayout *layout = new QVBoxLayout(panel);

    layout->addWidget(createCADControls(panel));
    layout->addWidget(createDocumentControls(panel));
    layout->addWidget(createCameraControls(panel));

    layout->addStretch();

    return panel;
}

QWidget *CADDemo::createCADControls(QWidget *parent)
{
    QGroupBox *group = new QGroupBox("View Options", parent);
    QVBoxLayout *layout = new QVBoxLayout(group);

    // ✅ 2D/3D 模式切换
    QGroupBox *modeGroup = new QGroupBox("View Mode");
    QVBoxLayout *modeLayout = new QVBoxLayout(modeGroup);

    QRadioButton *mode2D = new QRadioButton("2D Mode");
    QRadioButton *mode3D = new QRadioButton("3D Mode");
    mode2D->setChecked(camera->is2D());
    mode3D->setChecked(!camera->is2D());

    connect(mode2D, &QRadioButton::toggled, this, &CADDemo::switch2DMode);

    modeLayout->addWidget(mode2D);
    modeLayout->addWidget(mode3D);
    layout->addWidget(modeGroup);

    // ✅ 2D 视图方向选择
    QGroupBox *viewGroup = new QGroupBox("2D Views");
    QVBoxLayout *viewLayout = new QVBoxLayout(viewGroup);

    QPushButton *topBtn = new QPushButton("Top View (XY)");
    QPushButton *frontBtn = new QPushButton("Front View (XZ)");
    QPushButton *rightBtn = new QPushButton("Right View (YZ)");

    QGroupBox *drawGroup = new QGroupBox("Draw Type");
    QVBoxLayout *drawLayout = new QVBoxLayout(drawGroup);
    QButtonGroup *drawModeGroup = new QButtonGroup(drawLayout);
    drawModeGroup->setExclusive(true);

    QRadioButton *drawSelect = new QRadioButton("Select");
    drawSelect->setChecked(true);
    drawModeGroup->addButton(drawSelect, (int)DrawMode::SELECT);
    QRadioButton *drawLine = new QRadioButton("Line");
    drawModeGroup->addButton(drawLine, (int)DrawMode::LINE);
    QRadioButton *drawBox = new QRadioButton("Box");
    drawModeGroup->addButton(drawLine, (int)DrawMode::BOX);
    drawLayout->addWidget(drawSelect);
    drawLayout->addWidget(drawLine);
    drawLayout->addWidget(drawBox);
    layout->addWidget(drawGroup);
    connect(drawModeGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, &CADDemo::onDrawModeChanged);

    connect(topBtn, &QPushButton::clicked, [this]()
            { setViewOrientation((int)View2DOrientation::TOP); });
    connect(frontBtn, &QPushButton::clicked, [this]()
            { setViewOrientation((int)View2DOrientation::FRONT); });
    connect(rightBtn, &QPushButton::clicked, [this]()
            { setViewOrientation((int)View2DOrientation::RIGHT); });

    viewLayout->addWidget(topBtn);
    viewLayout->addWidget(frontBtn);
    viewLayout->addWidget(rightBtn);
    layout->addWidget(viewGroup);

    // ✅ 3D 视图
    QPushButton *isoBtn = new QPushButton("Isometric View (3D)");
    connect(isoBtn, &QPushButton::clicked, this, &CADDemo::setIsometricView);
    layout->addWidget(isoBtn);

    // 网格和坐标轴
    QCheckBox *gridCheckBox = new QCheckBox("Show Grid");
    // 先连接
    connect(gridCheckBox, &QCheckBox::clicked, this, &CADDemo::setGridVisible);
    // 后设置状态（这样点击才会正常工作）
    gridCheckBox->setChecked(showGrid_);
    layout->addWidget(gridCheckBox);

    // 同样处理 axis checkbox
    QCheckBox *axisCheckBox = new QCheckBox("Show Axis");
    connect(axisCheckBox, &QCheckBox::toggled, this, &CADDemo::setAxisVisible);
    axisCheckBox->setChecked(showAxis_);
    layout->addWidget(axisCheckBox);

    // 重置视图
    QPushButton *resetViewBtn = new QPushButton("Reset View");
    connect(resetViewBtn, &QPushButton::clicked, this, &CADDemo::resetView);
    layout->addWidget(resetViewBtn);

    // ✅ 统计信息（使用 QPointer 修复）
    QLabel *statsLabel = new QLabel();
    layout->addWidget(statsLabel);

    QPointer<QLabel> statsPtr(statsLabel);
    auto updateStats = [this, statsPtr]()
    {
        if (!statsPtr)
            return;

        int entityCount = document_->all().size();
        QString mode = camera->is2D() ? "2D" : "3D";
        statsPtr->setText(QString("Mode: %1\nEntities: %2\nWorld/Pixel: %3")
                              .arg(mode)
                              .arg(entityCount)
                              .arg(viewportState_.worldPerPixel, 0, 'f', 4));
    };

    updateStats();
    connect(this, &CADDemo::documentChanged, this, updateStats);
    connect(this, &CADDemo::parameterChanged, this, updateStats);

    return group;
}

QWidget *CADDemo::createDocumentControls(QWidget *parent)
{
    QGroupBox *group = new QGroupBox("Document", parent);
    QVBoxLayout *layout = new QVBoxLayout(group);

    QPushButton *addTestBtn = new QPushButton("Add Test Entities");
    connect(addTestBtn, &QPushButton::clicked, this, &CADDemo::addTestEntities);
    layout->addWidget(addTestBtn);

    QPushButton *clearBtn = new QPushButton("Clear Document");
    connect(clearBtn, &QPushButton::clicked, this, &CADDemo::clearDocument);
    layout->addWidget(clearBtn);

    return group;
}

// ============================================
// Slots 实现
// ============================================

void CADDemo::setWorkPlaneXY()
{
    workPlane_->setXY();
    emit statusMessage("Work plane: XY (Top)");
}

void CADDemo::setWorkPlaneXZ()
{
    workPlane_->setXZ();
    emit statusMessage("Work plane: XZ (Front)");
}

void CADDemo::setWorkPlaneYZ()
{
    workPlane_->setYZ();
    emit statusMessage("Work plane: YZ (Side)");
}

void CADDemo::setWorkPlaneFromView()
{
    workPlane_->setFromView(
        camera->getPosition(),
        camera->getFront(),
        camera->getTarget());
    emit statusMessage("Work plane: View Plane");
}

void CADDemo::toggleWorkPlaneFollow(bool enable)
{
    WorkPlane::FollowMode mode = workPlane_->getFollowMode();
    mode.enabled = enable;
    mode.followPosition = enable;
    mode.followOrientation = enable;
    workPlane_->setFollowMode(mode);

    emit statusMessage(enable ? "Work plane following view"
                              : "Work plane fixed");
}

void CADDemo::offsetWorkPlane(float distance)
{
    workPlane_->moveAlongNormal(distance);
    emit statusMessage(QString("Work plane offset: %1").arg(distance));
}
