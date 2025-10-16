#ifndef CADDEMO_H
#define CADDEMO_H

#include "Demo.h"
#include "../cad/data/document.h"
#include "../cad/data/renderer.h"
#include "../cad/data/GridAxisHelper.h"
#include <memory>

/**
 * CAD Demo - 基于 Document/Renderer 的 CAD 演示
 * 专门用于 2D/3D CAD 绘图应用
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
        return "2D/3D CAD viewer with document management"; 
    }

    // ============================================
    // CAD 特定的输入处理
    // ============================================
    
    void processKeyPress(CameraMovement qtKey, float deltaTime) override;
    void processMousePress(QPoint point) override;
    void processMouseMove(QPoint pos) override;
    void processMouseRelease() override;
    void processMouseWheel(int offset) override;
    void resizeViewport(int width, int height) override;

    // ============================================
    // CAD 文档访问
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
    
    // 测试用：添加一些示例实体
    void addTestEntities();
    void clearDocument();

signals:
    void documentChanged();
    void selectionChanged();

private:
    // ============================================
    // 辅助函数
    // ============================================
    
    void updateViewportState();
    void syncRendererFromDocument();
    
    QWidget* createCADControls(QWidget *parent = nullptr);
    QWidget* createDocumentControls(QWidget *parent = nullptr);

    // ============================================
    // CAD 核心组件
    // ============================================
    
    std::unique_ptr<Document> document_;
    std::unique_ptr<Renderer> renderer_;
    std::unique_ptr<GridRenderer> gridRenderer_;
    std::unique_ptr<AxisRenderer> axisRenderer_;
    
    // 视口状态
    ViewportState viewportState_;
    
    // 显示选项
    bool showGrid_;
    bool showAxis_;
    bool documentDirty_;  // 文档是否有变化
    
    // 鼠标交互状态
    bool isPanning_;
    QPoint lastMousePos_;
};

#endif // CADDEMO_H