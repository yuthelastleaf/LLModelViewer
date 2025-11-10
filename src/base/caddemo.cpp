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
#include <QApplication>
#include <QKeyEvent>
#include <glm/gtc/matrix_transform.hpp>

CADDemo::CADDemo(QObject *parent)
    : Demo(parent), 
      document_(std::make_unique<Document>()), 
      renderer_(std::make_unique<Renderer>()), 
      gridRenderer_(std::make_unique<GridRenderer>()), 
      axisRenderer_(std::make_unique<AxisRenderer>()), 
      showGrid_(true), 
      showAxis_(true), 
      documentDirty_(true), 
      isPanning_(false), 
      cad_mode_(DrawMode::VIEW), 
      cur_draw_(0), 
      selectionSystem_(std::make_unique<SelectionSystem>(document_.get(), this)), 
      cur_select_box_(0), 
      isBoxSelecting_(false), 
      isTransforming_(false), 
      draggedAxisIndex_(-1)
{
    // ✅ 默认设置为 2D CAD 俯视图
    camera->SetTarget(glm::vec3(0.0f, 0.0f, 0.0f));
    camera->SetTopView(10.0f);

    workPlane_ = std::make_unique<WorkPlane>();
    if (workPlane_)
    {
        // 默认 XY 平面
        workPlane_->setXY();

        // 启用跟随模式
        WorkPlane::FollowMode mode;
        mode.enabled = true;
        mode.followPosition = true;    // 原点跟随相机目标
        mode.followOrientation = true; // 法向量垂直于视线
        workPlane_->setFollowMode(mode);
    }

    // ✅ 连接选择信号
    connect(selectionSystem_.get(), &SelectionSystem::selectionChanged,
            this, [this](int count)
            { 
                emit statusMessage(QString("Selected: %1 entities").arg(count));
                
                // ✅ v0.3: 更新 Gizmo 显示
                if (count > 0 && cad_mode_ == DrawMode::MOVE) {
                    auto selectedEntities = selectionSystem_->getSelectedEntities();
                    glm::vec3 center = Transform::getSelectionCenter(selectedEntities);
                    createGizmo(center);
                } else {
                    destroyGizmo();
                } });

    // ✅ 注册实体删除回调，自动清理 GPU 批次
    document_->setRemoveCallback([this](EntityId id)
                                 { renderer_->removeBatch(id); });

    // ✅ 连接命令管理器信号
    auto& cmdMgr = CommandManager::instance();
    connect(&cmdMgr, &CommandManager::stackChanged, 
            this, &CADDemo::onCommandStackChanged);
    connect(&cmdMgr, &CommandManager::memoryWarning,
            this, [this](size_t currentMB, size_t limitMB) {
                emit statusMessage(QString("Memory warning: %1MB/%2MB").arg(currentMB).arg(limitMB));
            });
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
    // ✅ 初始化样式管理器
    RenderStyleManager::instance().initializeDefaultStyles();
    
    if (!renderer_->initialize())
    {
        emit statusMessage("Failed to initialize renderer");
        return;
    }

    // ✅ 确保视口状态已正确初始化（继承自Demo基类的updateViewportState）
    updateViewportState();

    addTestEntities();
    emit statusMessage("CAD Demo initialized");
}

void CADDemo::update(float deltaTime)
{
    // ❌ 不要在这里同步，在 render() 中统一处理
}

void CADDemo::render()
{
    updateViewportState();

    // ✅ 总是检查并同步文档变化（包括窗口大小变化和实体更新）
    if (documentDirty_)
    {
        // 窗口大小改变时强制重建，因为投影矩阵变化需要重新细分圆弧
        renderer_->syncFromDocument(*document_, viewportState_, viewportResized_);
        
        // ✅ 同步选择状态到Renderer（支持多实体悬停）
        renderer_->updateAllEntityStates(
            selectionSystem_->getSelectedIds(), 
            selectionSystem_->getHoveredIds()
        );

        // 清除所有实体的脏标志
        document_->clearAllDirtyFlags();
        document_->clearAllDirtyFlags();

        documentDirty_ = false;
        viewportResized_ = false;
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

    // 绘制文档实体（包括 Gizmo）
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

// ✅ 快捷键处理（Ctrl+Z, Ctrl+Y等）
bool CADDemo::handleKeyboardShortcut(QKeyEvent* event) {
    if (!event) return false;
    
    auto& cmdMgr = CommandManager::instance();
    
    // 检查修饰键
    bool ctrlPressed = (event->modifiers() & Qt::ControlModifier);
    bool shiftPressed = (event->modifiers() & Qt::ShiftModifier);
    
    if (ctrlPressed) {
        switch (event->key()) {
            case Qt::Key_Z:
                if (!shiftPressed) {
                    // Ctrl+Z: 撤销
                    if (cmdMgr.undo()) {
                        emit statusMessage("Undo: " + cmdMgr.getRedoText());
                        documentDirty_ = true;
                        return true;
                    } else {
                        emit statusMessage("Nothing to undo");
                        return true;
                    }
                } else {
                    // Ctrl+Shift+Z: 重做
                    if (cmdMgr.redo()) {
                        emit statusMessage("Redo: " + cmdMgr.getUndoText());
                        documentDirty_ = true;
                        return true;
                    } else {
                        emit statusMessage("Nothing to redo");
                        return true;
                    }
                }
                break;
                
            case Qt::Key_Y:
                // Ctrl+Y: 重做
                if (cmdMgr.redo()) {
                    emit statusMessage("Redo: " + cmdMgr.getUndoText());
                    documentDirty_ = true;
                    return true;
                } else {
                    emit statusMessage("Nothing to redo");
                    return true;
                }
                break;
                
            case Qt::Key_S:
                // Ctrl+S: 保存（创建保存点）
                cmdMgr.createSavePoint();
                emit statusMessage("Document saved - Command history cleared");
                return true;
                
            case Qt::Key_Delete:
            case Qt::Key_D:
                // Ctrl+D 或 Delete: 删除选中对象
                if (selectionSystem_->hasSelection()) {
                    deleteSelectedEntities();
                    return true;
                }
                break;
        }
    }
    
    // 单独的Delete键
    if (event->key() == Qt::Key_Delete && !ctrlPressed) {
        if (selectionSystem_->hasSelection()) {
            deleteSelectedEntities();
            return true;
        }
    }
    
    return false; // 未处理
}

void CADDemo::processMousePress(QPoint point, glm::vec3 wpoint)
{
    if (!getWorldPosition(point, wpoint))
    {
        return;
    }

    if (isPanning_)
    {
        return;
    }
    isPanning_ = true;
    switch (cad_mode_)
    {
    case DrawMode::VIEW:
        break;
    case DrawMode::LINE:
    {
        cur_draw_ = document_->addLine(wpoint, wpoint,
                                       Style::fromRGBA(0, 255, 0, 255));

        emit documentChanged();
        break;
    }

    case DrawMode::CIRCLE:
        emit statusMessage("Circle tool selected - Click to set center");
        break;

    case DrawMode::RECT:
        emit statusMessage("Rectangle tool selected - Click to set first corner");
        break;
    case DrawMode::BOX:
    {

        if (camera->is2D())
        {
            break;
        }

        Style boxStyle = Style::fromRGBA(100, 149, 237, 255);
        // ✅ 生成射线：只传递矩阵，不依赖 Camera 类
        Ray ray = Ray::fromScreen(
            point.x(), point.y(),
            viewportWidth, viewportHeight,
            camera->getViewMatrix(),
            camera->getProjectionMatrix(
                static_cast<float>(viewportWidth) / viewportHeight));
        glm::vec3 centerPos;
        // ✅ 与工作平面求交：只传递平面参数，不依赖 WorkPlane 类
        if (!ray.intersectPlane(
                workPlane_->getOrigin(),
                workPlane_->getNormal(),
                centerPos))
        {
            emit statusMessage("Cannot project to work plane");
            break;
        }
        EntityId boxId = document_->addBox(centerPos, 1.0f, boxStyle);
        documentDirty_ = true; // ✅ 标记需要立即渲染
        emit documentChanged();
    }
    break;
    case DrawMode::SELECT:
    {
        // 获取键盘修饰键
        Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();

        // 判断选择模式
        SelectionSystem::SelectMode selectMode = SelectionSystem::SelectMode::REPLACE;
        if (modifiers & Qt::ShiftModifier)
        {
            selectMode = SelectionSystem::SelectMode::ADD;
        }
        else if (modifiers & Qt::ControlModifier)
        {
            selectMode = SelectionSystem::SelectMode::TOGGLE;
        }

        // ✅ 根据相机模式选择拾取方式 - 使用统一拾取方法
        std::optional<SelectionSystem::PickResult> pickResult;

        // ✅ 首先检查是否在MOVE模式且点击了Gizmo轴
        if (cad_mode_ == DrawMode::MOVE && !gizmoAxisIds_.empty()) {
            Ray ray = Ray::fromScreen(
                point.x(), point.y(),
                viewportState_.width, viewportState_.height,
                viewportState_.view, viewportState_.proj);

            int gizmoAxis = selectionSystem_->pickGizmoAxis(ray, gizmoSize_ * 0.5f);
            if (gizmoAxis >= 0 && gizmoAxis < gizmoAxisIds_.size()) {
                // 如果点击了Gizmo轴，创建一个对应的PickResult
                pickResult = SelectionSystem::PickResult{gizmoAxisIds_[gizmoAxis], glm::vec3(0), 0.0f};
            }
        }
        
        // 如果没有拾取到Gizmo轴，使用常规拾取
        if (!pickResult.has_value()) {
            pickResult = selectionSystem_->pickUnified(point, viewportState_, 5.0f);
        }

        // ✅ 调试：检查hover和click的一致性
        if (point == lastHoverPos_ && pickResult.has_value() && 
            pickResult->entityId != lastHoveredId_) {
            qDebug() << "⚠️  Hover/Click不一致！Hover:" << lastHoveredId_ 
                     << "Click:" << pickResult->entityId 
                     << "位置:" << point.x() << point.y();
        }

        // 处理拾取结果
        if (pickResult.has_value())
        {
            selectionSystem_->selectWithMode(pickResult->entityId, selectMode);
            documentDirty_ = true;
        }
        else
        {
            if (selectMode == SelectionSystem::SelectMode::REPLACE)
            {
                selectionSystem_->clearSelection();
                documentDirty_ = true;
            }
        }

        break;
    }
    case DrawMode::SELECTBOX:
    {
        cur_select_box_ = document_->addRectangle(wpoint, wpoint, true, Style::fromRGBA(255, 255, 255, 255), true);
        isBoxSelecting_ = true;
        break;
    }

    case DrawMode::MOVE:
    {
        // ✅ v0.3: 移动模式
        if (!selectionSystem_->hasSelection())
        {
            emit statusMessage("No selection to move");
            break;
        }

        // 检查是否点击了 Gizmo 轴（暂时移除，因为GizmoAxis拾取未实现）
        // ✅ 实现 Gizmo 轴拾取
        Ray ray = Ray::fromScreen(
            point.x(), point.y(),
            viewportState_.width, viewportState_.height,
            viewportState_.view, viewportState_.proj);

        draggedAxisIndex_ = selectionSystem_->pickGizmoAxis(ray, gizmoSize_ * 0.5f); // ✅ 放宽拾取容忍度

        if (draggedAxisIndex_ >= 0)
        {
            isTransforming_ = true;
            transformStartPos_ = wpoint;
            transformOffset_ = glm::vec3(0.0f);

            const char *axisNames[] = {"X", "Y", "Z"};
            emit statusMessage(QString("Moving along %1 axis...").arg(axisNames[draggedAxisIndex_]));
        }
        break;
    }
    }
}

void CADDemo::processMouseMove(QPoint point, QPoint delta_point, glm::vec3 wpoint, glm::vec3 delta_wpoint)
{
    if (!getWorldPosition(point, wpoint))
    {
        // 投影失败，使用传入的默认值
    }

    switch (cad_mode_)
    {
    case DrawMode::SELECT:
    {
        // 清除之前的悬停状态
        selectionSystem_->clearHovered();

        // ✅ 检测当前悬停的实体 - 使用统一拾取方法
        std::optional<SelectionSystem::PickResult> pickResult;

        // ✅ 首先检查是否在MOVE模式区域内且有Gizmo轴
        if (cad_mode_ == DrawMode::MOVE && !gizmoAxisIds_.empty()) {
            Ray ray = Ray::fromScreen(
                point.x(), point.y(),
                viewportState_.width, viewportState_.height,
                viewportState_.view, viewportState_.proj);

            int gizmoAxis = selectionSystem_->pickGizmoAxis(ray, gizmoSize_ * 0.5f);
            if (gizmoAxis >= 0 && gizmoAxis < gizmoAxisIds_.size()) {
                pickResult = SelectionSystem::PickResult{gizmoAxisIds_[gizmoAxis], glm::vec3(0), 0.0f};
            }
        }
        
        // 如果没有拾取到Gizmo轴，使用常规拾取
        if (!pickResult.has_value()) {
            pickResult = selectionSystem_->pickUnified(point, viewportState_, 5.0f);
        }

        if (pickResult.has_value())
        {
            selectionSystem_->setHovered(pickResult->entityId);
            documentDirty_ = true;

            // ✅ 记录hover状态用于调试
            lastHoveredId_ = pickResult->entityId;
            lastHoverPos_ = point;

            // 显示实体信息
            emit statusMessage(QString("Hover: Entity %1").arg(pickResult->entityId));
        }
        else
        {
            lastHoveredId_ = EntityId(-1);
        }
        break;
    }
    
    case DrawMode::MOVE:
    {
        // ✅ MOVE 模式：检测 Gizmo 轴的 hover 状态
        if (!isTransforming_) // 只有在非拖拽状态下才检测 hover
        {
            // 清除之前的悬停状态
            selectionSystem_->clearHovered();

            // 检测 Gizmo 轴 hover
            Ray ray = Ray::fromScreen(
                point.x(), point.y(),
                viewportState_.width, viewportState_.height,
                viewportState_.view, viewportState_.proj);

            int hoveredAxis = selectionSystem_->pickGizmoAxis(ray, gizmoSize_ * 0.5f);
            
            if (hoveredAxis >= 0)
            {
                // 找到对应的 Gizmo 轴实体并设置 hover
                if (hoveredAxis < gizmoAxisIds_.size())
                {
                    selectionSystem_->setHovered(gizmoAxisIds_[hoveredAxis]);
                    documentDirty_ = true;
                    
                    const char *axisNames[] = {"X", "Y", "Z"};
                    emit statusMessage(QString("Hover: %1 Axis - Click to move along this axis").arg(axisNames[hoveredAxis]));
                }
            }
        }
        break;
    }
    }

    if (!isPanning_)
    {
        return;
    }

    switch (cad_mode_)
    {
    case DrawMode::VIEW:
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
            document_->updateEndLinePoint(cur_draw_, wpoint);
            documentDirty_ = true;
            emit documentChanged();
        }
        break;

    case DrawMode::CIRCLE:
        emit statusMessage("Circle tool selected - Click to set center");
        break;

    case DrawMode::RECT:
        emit statusMessage("Rectangle tool selected - Click to set first corner");
        break;
    case DrawMode::SELECTBOX:
        if (isBoxSelecting_)
        {
            document_->updateEndLinePoint(cur_select_box_, wpoint);

            // ⭐ 根据相机模式选择 2D 或 3D 框选
            if (camera->is2D())
            {
                // 2D 模式：直接在屏幕空间框选
                std::vector<EntityId> boxSelectedIds = selectionSystem_->pickByBox2D(
                    cur_select_box_,
                    SelectionSystem::BoxSelectMode::INTERSECT);
                
                // ✅ 设置悬停状态（预览）- 支持多实体悬停
                selectionSystem_->setHovered(boxSelectedIds);
            }
            else
            {
                // 3D 模式：在工作平面上使用射线投影框选
                std::vector<EntityId> boxSelectedIds = selectionSystem_->pickByBox3D(
                    cur_select_box_,
                    viewportState_,
                    *workPlane_,
                    SelectionSystem::BoxSelectMode::INTERSECT);
                
                // ✅ 3D 模式也设置悬停预览
                selectionSystem_->setHovered(boxSelectedIds);
            }

            documentDirty_ = true; // ✅ 标记需要重新同步
        }
        emit documentChanged();
        break;

    case DrawMode::MOVE:
    {
        // ✅ v0.3: 移动模式 - 实时预览
        if (isTransforming_)
        {
            // 计算偏移量（根据拖拽的轴限制移动方向）
            glm::vec3 currentOffset = wpoint - transformStartPos_;

            // 根据拖拽的轴过滤偏移量
            if (draggedAxisIndex_ == 0)
            { // X 轴
                currentOffset.y = 0.0f;
                currentOffset.z = 0.0f;
            }
            else if (draggedAxisIndex_ == 1)
            { // Y 轴
                currentOffset.x = 0.0f;
                currentOffset.z = 0.0f;
            }
            else if (draggedAxisIndex_ == 2)
            { // Z 轴
                currentOffset.x = 0.0f;
                currentOffset.y = 0.0f;
            }
            // 否则自由移动，不限制

            // 计算增量偏移
            glm::vec3 deltaOffset = currentOffset - transformOffset_;
            transformOffset_ = currentOffset;

            // 应用到选中的实体
            auto selectedEntities = selectionSystem_->getSelectedEntities();
            Transform::translateEntities(selectedEntities, deltaOffset);

            // 更新 Gizmo 位置
            glm::vec3 newCenter = Transform::getSelectionCenter(selectedEntities);
            updateGizmoPosition(newCenter);

            documentDirty_ = true;
            emit statusMessage(QString("Offset: %1, %2, %3")
                                   .arg(transformOffset_.x, 0, 'f', 2)
                                   .arg(transformOffset_.y, 0, 'f', 2)
                                   .arg(transformOffset_.z, 0, 'f', 2));
        }
        break;
    }
    }
}

void CADDemo::processMouseRelease()
{
    isPanning_ = false;

    // ============================================
    // ✅ v0.3: 移动完成
    // ============================================

    if (isTransforming_)
    {
        isTransforming_ = false;
        draggedAxisIndex_ = -1;

        emit statusMessage(QString("Move completed: offset (%1, %2, %3)")
                               .arg(transformOffset_.x, 0, 'f', 2)
                               .arg(transformOffset_.y, 0, 'f', 2)
                               .arg(transformOffset_.z, 0, 'f', 2));

        // ✅ 使用命令系统记录移动操作
        executeMoveCommand();

        // 变换完成，标记为非脏数据
        documentDirty_ = true;
        return;
    }

    // ============================================
    // ✅ v0.2: 框选完成
    // ============================================

    if (isBoxSelecting_)
    {
        isBoxSelecting_ = false;

        Entity *entity = document_->get(cur_select_box_);

        Rectangle *rect = entity ? std::get_if<Rectangle>(&entity->geom) : nullptr;
        if (!rect)
        {
            return;
        }

        std::vector<EntityId> selectedIds = selectionSystem_->pickByBox2D(
            cur_select_box_,
            SelectionSystem::BoxSelectMode::INTERSECT);

        Qt::KeyboardModifiers modifiers = QApplication::keyboardModifiers();
        SelectionSystem::SelectMode selectMode = SelectionSystem::SelectMode::REPLACE;

        if (modifiers & Qt::ShiftModifier)
        {
            selectMode = SelectionSystem::SelectMode::ADD;
        }
        else if (modifiers & Qt::ControlModifier)
        {
            selectMode = SelectionSystem::SelectMode::TOGGLE;
        }

        if (!selectedIds.empty())
        {
            // 设置选中状态
            selectionSystem_->selectWithMode(selectedIds, selectMode);
            documentDirty_ = true;
        }
        else
        {
            // ✅ 框选没有选到任何对象
            if (selectMode == SelectionSystem::SelectMode::REPLACE)
            {
                // 替换模式：清空选择
                selectionSystem_->clearSelection();
                documentDirty_ = true;
            }
            // ADD 和 TOGGLE 模式：保持现有选择不变
        }

        // 删除选择框
        entity = nullptr;
        document_->remove(cur_select_box_);
        emit documentChanged();
    }
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

    // 只有当宽度或高度变化超过 10 像素时才标记需要强制重建
    if (lastViewportWidth_ == 0 || lastViewportHeight_ == 0 ||
        std::abs(width - lastViewportWidth_) > 10 ||
        std::abs(height - lastViewportHeight_) > 10)
    {
        viewportResized_ = true;
        lastViewportWidth_ = width;
        lastViewportHeight_ = height;
    }

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
    if (enable)
    {
        camera->SetTopView();
    }
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
    emit statusMessage(QString("Added 6 test entities (including dotted rectangle)"));
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
    case DrawMode::VIEW:
        emit statusMessage("View tool active");
        destroyGizmo();
        break;

    case DrawMode::SELECT:
        emit statusMessage("Selection tool active");
        destroyGizmo();
        break;

    case DrawMode::MOVE:
        emit statusMessage("Move tool active - Select objects to move");
        if (selectionSystem_->hasSelection())
        {
            auto selectedEntities = selectionSystem_->getSelectedEntities();
            glm::vec3 center = Transform::getSelectionCenter(selectedEntities);
            createGizmo(center);
        }
        else
        {
            destroyGizmo();
        }
        break;

    case DrawMode::LINE:
        emit statusMessage("Line tool selected - Click to set start point");
        destroyGizmo();
        break;

    case DrawMode::CIRCLE:
        emit statusMessage("Circle tool selected - Click to set center");
        destroyGizmo();
        break;

    case DrawMode::RECT:
        emit statusMessage("Rectangle tool selected - Click to set first corner");
        destroyGizmo();
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

    for (int i = 0; i < static_cast<int>(DrawMode::COUNT); ++i)
    {
        DrawMode mode = static_cast<DrawMode>(i);
        QRadioButton *drawbtn = new QRadioButton(drawModeToString(mode));
        if (!i)
        {
            drawbtn->setChecked(true);
        }
        drawModeGroup->addButton(drawbtn, (int)mode);
        drawLayout->addWidget(drawbtn);
    }

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

const char *CADDemo::drawModeToString(DrawMode mode)
{
    switch (mode)
    {
    case DrawMode::VIEW:
        return "View";
    case DrawMode::SELECT:
        return "Select";
    case DrawMode::MOVE:
        return "Move";
    case DrawMode::LINE:
        return "Line";
    case DrawMode::CIRCLE:
        return "Circle";
    case DrawMode::RECT:
        return "Rect";
    case DrawMode::BOX:
        return "Box";
    case DrawMode::SELECTBOX:
        return "SelectBox";
    default:
        return "Unknown";
    }
}

// ============================================
// ✅ v0.2: 选择操作 Slots
// ============================================

void CADDemo::clearSelection()
{
    selectionSystem_->clearSelection();
    documentDirty_ = true;
}

void CADDemo::selectAll()
{
    selectionSystem_->selectAll();
    documentDirty_ = true;
}

void CADDemo::invertSelection()
{
    selectionSystem_->invertSelection();
    documentDirty_ = true;
}

void CADDemo::deleteSelected()
{
    auto selectedIds = selectionSystem_->getSelectedIds();

    if (selectedIds.empty())
    {
        emit statusMessage("No selection to delete");
        return;
    }

    for (EntityId id : selectedIds)
    {
        document_->remove(id);
        renderer_->removeBatch(id);
    }

    selectionSystem_->clearSelection();
    documentDirty_ = true;

    emit statusMessage(QString("Deleted %1 entities").arg(selectedIds.size()));
    emit documentChanged();
}

// ============================================
// ✅ v0.3: 坐标转换辅助方法
// ============================================

bool CADDemo::getWorldPosition(const QPoint &screenPos, glm::vec3 &outWorldPos) const
{
    if (camera->is2D())
    {
        // 2D 模式：直接使用屏幕坐标转世界坐标
        outWorldPos = viewportState_.screenToWorld(screenPos, 0.0f);
        return true;
    }
    else
    {
        // 3D 模式：射线与工作平面求交
        Ray ray = Ray::fromScreen(
            screenPos.x(), screenPos.y(),
            viewportState_.width, viewportState_.height,
            viewportState_.view, viewportState_.proj);

        return workPlane_->rayIntersection(
            ray.getOrigin(),
            ray.getDirection(),
            outWorldPos);
    }
}

// ============================================
// ✅ v0.3: Gizmo 辅助方法实现
// ============================================

void CADDemo::createGizmo(const glm::vec3 &center)
{
    // 先销毁旧的 Gizmo
    destroyGizmo();

    // ✅ 根据选中物体的大小和视口大小计算 Gizmo 大小
    updateGizmoSize(viewportState_);
    
    // ✅ 根据选中实体的包围盒调整 Gizmo 大小
    auto selectedEntities = selectionSystem_->getSelectedEntities();
    float objectScale = calculateSelectionBoundingBoxSize(selectedEntities);
    float adaptiveSize = std::max(gizmoSize_, objectScale * 0.3f); // 至少是物体大小的30%

    // 定义三个轴：X/Y/Z - 使用更明显的颜色
    glm::vec3 directions[] = {
        glm::vec3(1, 0, 0), // X 轴 - 红色
        glm::vec3(0, 1, 0), // Y 轴 - 绿色
        glm::vec3(0, 0, 1)  // Z 轴 - 蓝色
    };

    std::uint32_t colors[] = {
        0xFF4444FF, // 更亮的红色
        0x44FF44FF, // 更亮的绿色
        0x4444FFFF  // 更亮的蓝色
    };

    // 创建三个轴
    for (int i = 0; i < 3; ++i)
    {
        EntityId axisId = document_->addGizmoAxis(
            center,
            directions[i],
            adaptiveSize, // ✅ 使用自适应大小
            i,
            Style{colors[i]});
        gizmoAxisIds_.push_back(axisId);
    }

    documentDirty_ = true;
}

void CADDemo::destroyGizmo()
{
    for (EntityId id : gizmoAxisIds_)
    {
        document_->remove(id);
    }
    gizmoAxisIds_.clear();
    documentDirty_ = true;
}

void CADDemo::updateGizmoPosition(const glm::vec3 &center)
{
    for (size_t i = 0; i < gizmoAxisIds_.size() && i < 3; ++i)
    {
        Entity *entity = document_->get(gizmoAxisIds_[i]);
        if (entity && entity->type == EntityType::GizmoAxis)
        {
            auto *gizmo = std::get_if<GizmoAxis>(&entity->geom);
            if (gizmo)
            {
                gizmo->origin = center;
                entity->dirty = true;
            }
        }
    }
}

void CADDemo::updateGizmoSize(const ViewportState &vp)
{
    // 根据屏幕大小计算世界空间中的 Gizmo 大小
    // 目标：Gizmo 在屏幕上保持 120 像素左右的大小（增加了大小）
    gizmoSize_ = 120.0f * vp.worldPerPixel;
}

float CADDemo::calculateSelectionBoundingBoxSize(const std::vector<Entity*>& entities) const
{
    if (entities.empty()) return 1.0f;
    
    // 计算所有选中实体的包围盒
    glm::vec3 minPoint(std::numeric_limits<float>::max());
    glm::vec3 maxPoint(std::numeric_limits<float>::lowest());
    
    for (const auto* entity : entities) {
        if (!entity || entity->isGizmo) continue;
        
        switch (entity->type) {
            case EntityType::Line: {
                if (auto* line = std::get_if<Line>(&entity->geom)) {
                    minPoint = glm::min(minPoint, glm::min(line->p0, line->p1));
                    maxPoint = glm::max(maxPoint, glm::max(line->p0, line->p1));
                }
                break;
            }
            case EntityType::Circle: {
                if (auto* circle = std::get_if<Circle>(&entity->geom)) {
                    glm::vec3 r(circle->r);
                    minPoint = glm::min(minPoint, circle->c - r);
                    maxPoint = glm::max(maxPoint, circle->c + r);
                }
                break;
            }
            case EntityType::Box: {
                if (auto* box = std::get_if<Box>(&entity->geom)) {
                    float half = box->size * 0.5f;
                    glm::vec3 halfVec(half);
                    minPoint = glm::min(minPoint, box->center - halfVec);
                    maxPoint = glm::max(maxPoint, box->center + halfVec);
                }
                break;
            }
            case EntityType::Polyline: {
                if (auto* polyline = std::get_if<Polyline>(&entity->geom)) {
                    for (const auto& pt : polyline->pts) {
                        minPoint = glm::min(minPoint, pt);
                        maxPoint = glm::max(maxPoint, pt);
                    }
                }
                break;
            }
            // 可以添加其他类型...
        }
    }
    
    // 计算包围盒的最大尺寸
    glm::vec3 size = maxPoint - minPoint;
    return std::max({size.x, size.y, size.z, 0.5f}); // 最小返回 0.5
}

// ============================================
// ✅ v0.4: 命令系统集成
// ============================================

void CADDemo::deleteSelectedEntities() {
    auto selectedIds = selectionSystem_->getSelectedIds();
    if (selectedIds.empty()) {
        emit statusMessage("No entities selected to delete");
        return;
    }
    
    // 转换为vector（命令系统需要vector类型）
    std::vector<EntityId> idVector(selectedIds.begin(), selectedIds.end());
    
    // 创建删除命令
    auto deleteCmd = std::make_unique<DeleteEntityCommand>(document_.get(), idVector);
    
    // 执行命令
    auto& cmdMgr = CommandManager::instance();
    if (cmdMgr.executeCommand(std::move(deleteCmd))) {
        selectionSystem_->clearSelection();
        documentDirty_ = true;
        emit statusMessage(QString("Deleted %1 entities").arg(selectedIds.size()));
    } else {
        emit statusMessage("Failed to delete entities");
    }
}

void CADDemo::executeMoveCommand() {
    // 检查是否有实际的移动
    const float epsilon = 1e-6f;
    if (glm::length(transformOffset_) < epsilon) {
        return; // 没有移动，不创建命令
    }
    
    auto selectedIds = selectionSystem_->getSelectedIds();
    if (selectedIds.empty()) {
        return;
    }
    
    // 转换为vector（命令系统需要vector类型）
    std::vector<EntityId> idVector(selectedIds.begin(), selectedIds.end());
    
    // 创建移动命令
    auto moveCmd = std::make_unique<MoveCommand>(document_.get(), idVector, transformOffset_);
    
    // 由于移动已经在实时预览中执行过了，我们需要先撤销然后通过命令执行
    // 撤销实时预览的移动
    auto selectedEntities = selectionSystem_->getSelectedEntities();
    Transform::translateEntities(selectedEntities, -transformOffset_);
    
    // 通过命令系统执行移动
    auto& cmdMgr = CommandManager::instance();
    if (cmdMgr.executeCommand(std::move(moveCmd), true)) { // 允许合并连续移动
        // 重置移动状态
        transformOffset_ = glm::vec3(0.0f);
    } else {
        // 如果命令执行失败，恢复实时预览的移动
        Transform::translateEntities(selectedEntities, transformOffset_);
        emit statusMessage("Failed to record move command");
    }
}

void CADDemo::onCommandStackChanged() {
    auto& cmdMgr = CommandManager::instance();
    
    // 更新状态栏显示
    QString statusText;
    if (cmdMgr.canUndo()) {
        statusText += QString("[%1] ").arg(cmdMgr.getUndoText());
    }
    if (cmdMgr.canRedo()) {
        statusText += QString("| %1").arg(cmdMgr.getRedoText());
    }
    
    if (!statusText.isEmpty()) {
        emit statusMessage(statusText);
    }
    
    // 如果有UI的话，这里可以更新undo/redo按钮状态
    // updateUndoRedoButtons();
}
