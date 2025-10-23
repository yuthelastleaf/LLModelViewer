#ifndef CADDEMO_H
#define CADDEMO_H

#include "Demo.h"
#include "../cad/data/document.h"
#include "../cad/data/renderer.h"
#include "../cad/data/GridAxisHelper.h"
#include "util/RayUtils.h"
#include "util/WorkPlane.h"
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
        SELECT = 0,
        LINE = 1,
        CIRCLE = 2,
        RECT = 3,
        BOX
    };

private:
    // ============================================
    // 核心组件
    // ============================================
    
    std::unique_ptr<Document> document_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<GridRenderer> gridRenderer_;
    std::unique_ptr<AxisRenderer> axisRenderer_;

    EntityId cur_draw_;
    DrawMode cad_mode_;
    
    // ❌ 删除重复声明：viewportState_ 已在基类中
    
    // 显示选项
    bool showGrid_;
    bool showAxis_;
    bool documentDirty_;
    
    // 鼠标交互
    bool isPanning_;

    std::unique_ptr<WorkPlane> workPlane_;
};

#endif // CADDEMO_H