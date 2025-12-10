#ifndef CADDEMO_H
#define CADDEMO_H

#include "Demo.h"
#include "../cad/data/document.h"
#include "../cad/data/renderer.h"
#include "../cad/data/GridAxisHelper.h"
#include "../cad/selection/SelectionSystem.h"
#include "../cad/transform/Transform.h"
#include "../cad/command/CommandManager.h"
#include "../cad/command/EntityCommands.h"
#include "util/RayUtils.h"
#include "util/WorkPlane.h"
#include "util/RenderStyleManager.h"
#include <memory>

/**
 * CAD Demo - 支持 2D/3D 模式切换的 CAD 应用
 */
class CADDemo : public Demo
{
    Q_OBJECT

public:
    explicit CADDemo(QObject *parent = nullptr);
    virtual ~CADDemo() override;

    // ============================================
    // 重写 Demo 基类接口
    // ============================================
    
    void initialize() override;
    void update(float deltaTime) override;
    void render() override;
    void cleanup() override;
    
    QString getName() const override { return "CAD Viewer"; }
    QString getDescription() const override { 
        return "2D/3D CAD viewer with mode switching"; 
    }

    // ============================================
    // 输入处理
    // ============================================
    
    void processKeyPress(CameraMovement qtKey, float deltaTime) override;
    bool handleKeyboardShortcut(QKeyEvent* event); // ✅ 新增：快捷键处理（Ctrl+Z等）
    void processMousePress(QPoint point, glm::vec3 wpoint) override;
    void processMouseMove(QPoint point, QPoint delta_point, glm::vec3 wpoint, glm::vec3 delta_wpoint) override;
    void processMouseRelease() override;
    void processMouseWheel(int offset) override;
    void resizeViewport(int width, int height) override;

    // ============================================
    // 文档访问
    // ============================================
    
    Document* getDocument() { return document_.get(); }
    const Document* getDocument() const { return document_.get(); }
    
    Renderer* getRenderer() { return renderer_.get(); }
    const Renderer* getRenderer() const { return renderer_.get(); }

    // ============================================
    // 控制面板
    // ============================================
    
    QWidget* createControlPanel(QWidget *parent = nullptr) override;

public slots:
    // 视图控制
    void setGridVisible(bool visible);
    void setAxisVisible(bool visible);
    void resetView();
    
    // 2D/3D 模式切换
    void switch2DMode(bool enable);
    void setViewOrientation(int orientation);  // 0=Top, 1=Front, 2=Right
    void setIsometricView();
    
    // 文档操作
    void addTestEntities();
    void clearDocument();

    // 绘制模式切换
    void onDrawModeChanged(int id);

    // 工作平面控制
    void setWorkPlaneXY();
    void setWorkPlaneXZ();
    void setWorkPlaneYZ();
    void setWorkPlaneFromView();
    void toggleWorkPlaneFollow(bool enable);
    void offsetWorkPlane(float distance);

    // ✅ v0.2: 选择操作
    void clearSelection();
    void selectAll();
    void invertSelection();
    void deleteSelected();
    
    // ✨ v0.3: 视觉效果配置
    void setHoverQuality(int quality); // 0=Basic, 1=Enhanced, 2=Premium

    // ✅ v0.4: 命令系统槽
    void onCommandStackChanged(); // 响应命令栈变化

signals:
    void documentChanged();
    void selectionChanged();

protected:
    // ✅ 重写基类方法以支持 2D/3D 模式切换
    void updateViewportState() override;

private:
    // ============================================
    // 辅助函数
    // ============================================
    
    void syncRendererFromDocument();
    
    QWidget* createCADControls(QWidget *parent = nullptr);
    QWidget* createDocumentControls(QWidget *parent = nullptr);

    // 绘图状态
    enum class DrawMode {
        VIEW = 0,
        SELECT,
        MOVE,        // ✅ 新增：移动模式
        LINE,
        CIRCLE,
        RECT,
        BOX,
        SELECTBOX,
        COUNT
    };

    const char* drawModeToString(DrawMode mode);
    
    /**
     * 将屏幕坐标转换为世界坐标（考虑2D/3D模式和工作平面）
     * @param screenPos 屏幕坐标
     * @param outWorldPos [输出] 世界坐标
     * @return 是否成功转换
     */
    bool getWorldPosition(const QPoint& screenPos, glm::vec3& outWorldPos) const;

private:
    // ============================================
    // 核心组件
    // ============================================
    
    std::unique_ptr<Document> document_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<GridRenderer> gridRenderer_;
    std::unique_ptr<AxisRenderer> axisRenderer_;

    EntityId cur_draw_;
    EntityId cur_select_box_;
    DrawMode cad_mode_;
    
    // ❌ 删除重复声明：viewportState_ 已在基类中
    
    // 显示选项
    bool showGrid_;
    bool showAxis_;
    bool documentDirty_;
    bool viewportResized_ = false;  // ✅ 标记窗口大小是否改变
    int lastViewportWidth_ = 0;     // ✅ 上次的视口宽度
    int lastViewportHeight_ = 0;    // ✅ 上次的视口高度
    
    // 鼠标交互
    bool isPanning_;

    std::unique_ptr<WorkPlane> workPlane_;

    // ✅ v0.3: 统一选择系统（替代Picker+SelectionManager）
    std::unique_ptr<SelectionSystem> selectionSystem_;

    // 框选状态
    bool isBoxSelecting_ = false;
    
    // ✅ v0.3: 变换系统（使用 Document 管理 Gizmo）
    std::vector<EntityId> gizmoAxisIds_;    // Gizmo 轴的实体 ID（X/Y/Z）
    bool isTransforming_ = false;           // 是否正在变换
    int draggedAxisIndex_ = -1;             // 拖拽的轴索引（0=X, 1=Y, 2=Z，-1=无）
    glm::vec3 transformStartPos_;           // 变换开始时的鼠标位置
    glm::vec3 transformOffset_;             // 累积的变换偏移
    float gizmoSize_ = 1.0f;                // Gizmo 大小（世界单位）
    
    // ✅ 调试：追踪hover和click的一致性
    mutable EntityId lastHoveredId_ = EntityId(-1);
    mutable QPoint lastHoverPos_;
    
    // ✅ v0.4: 命令系统集成
    void deleteSelectedEntities();      // 删除选中实体（使用命令）
    void executeMoveCommand();          // 完成移动操作（使用命令）
    
    // Gizmo 辅助方法
    void createGizmo(const glm::vec3& center);
    void destroyGizmo();
    void updateGizmoPosition(const glm::vec3& center);
    void updateGizmoSize(const ViewportState& vp);
    float calculateSelectionBoundingBoxSize(const std::vector<Entity*>& entities) const;
};

#endif // CADDEMO_H