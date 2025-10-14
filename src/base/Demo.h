#ifndef DEMO_H
#define DEMO_H

#include <QObject>
#include <QString>
#include <QPoint>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// 前向声明
class Camera;
class LightManager;
// class GridAxisHelper;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class QWidget;

/**
 * Demo 基类 - Qt 版本
 * 所有 OpenGL 演示的基础接口
 * 继承自 QObject 以支持信号槽机制
 */
class Demo : public QObject
{
    Q_OBJECT

public:
    explicit Demo(QObject *parent = nullptr);
    virtual ~Demo();

    // ============================================
    // 核心虚函数 - 子类必须实现
    // ============================================
    
    /**
     * 初始化 Demo（创建着色器、VAO、VBO 等）
     * 在 OpenGL 上下文已创建后调用
     */
    virtual void initialize() = 0;
    
    /**
     * 更新逻辑（动画、物理等）
     * @param deltaTime 上一帧的时间间隔（秒）
     */
    virtual void update(float deltaTime) = 0;
    
    /**
     * 渲染场景
     * 在 paintGL() 中调用
     */
    virtual void render() = 0;
    
    /**
     * 清理资源（删除 VAO、VBO、着色器等）
     */
    virtual void cleanup() = 0;
    
    /**
     * 获取 Demo 名称（用于菜单显示）
     */
    virtual QString getName() const = 0;
    
    /**
     * 获取 Demo 描述
     */
    virtual QString getDescription() const { 
        return "No description available."; 
    }

    // ============================================
    // 输入处理 - 可选重写
    // ============================================
    
    /**
     * 处理键盘按键
     */
    virtual void processKeyPress(QKeyEvent *event);
    
    /**
     * 处理鼠标按下
     */
    virtual void processMousePress(QMouseEvent *event);
    
    /**
     * 处理鼠标移动
     */
    virtual void processMouseMove(QMouseEvent *event);
    
    /**
     * 处理鼠标释放
     */
    virtual void processMouseRelease(QMouseEvent *event);
    
    /**
     * 处理鼠标滚轮
     */
    virtual void processMouseWheel(QWheelEvent *event);
    
    /**
     * 处理窗口大小变化
     */
    virtual void resizeViewport(int width, int height);

    // ============================================
    // 访问器
    // ============================================
    
    Camera* getCamera() { return camera.get(); }
    const Camera* getCamera() const { return camera.get(); }
    
    LightManager* getLightManager() { return lightManager.get(); }
    const LightManager* getLightManager() const { return lightManager.get(); }
    
    // GridAxisHelper* getGridAxisHelper() { return gridAxisHelper.get(); }
    // const GridAxisHelper* getGridAxisHelper() const { return gridAxisHelper.get(); }
    
    int getViewportWidth() const { return viewportWidth; }
    int getViewportHeight() const { return viewportHeight; }

    // ============================================
    // 控制面板创建 - 子类可重写以添加自定义控件
    // ============================================
    
    /**
     * 创建 Qt 控制面板（替代 ImGui）
     * @return 返回控件指针，由调用者管理生命周期
     */
    virtual QWidget* createControlPanel(QWidget *parent = nullptr);

signals:
    // 状态变化信号
    void statusMessage(const QString &message);
    void parameterChanged();

protected:
    // ============================================
    // 辅助函数
    // ============================================
    
    /**
     * 获取视图矩阵
     */
    glm::mat4 getViewMatrix() const;
    
    /**
     * 获取背面视图矩阵（用于天空盒等）
     */
    glm::mat4 getBackViewMatrix() const;
    
    /**
     * 获取投影矩阵
     */
    glm::mat4 getProjectionMatrix() const;
    
    /**
     * 获取 MVP 矩阵
     */
    glm::mat4 getMVPMatrix(const glm::mat4 &model) const;

    // ============================================
    // 通用控制面板组件创建
    // ============================================
    
    /**
     * 创建相机控制组件
     */
    QWidget* createCameraControls(QWidget *parent = nullptr);
    
    /**
     * 创建光照控制组件
     */
    QWidget* createLightControls(QWidget *parent = nullptr);
    
    /**
     * 创建网格/坐标轴控制组件
     */
    QWidget* createGridAxisControls(QWidget *parent = nullptr);

    // ============================================
    // 成员变量
    // ============================================
    
    std::unique_ptr<Camera> camera;
    std::unique_ptr<LightManager> lightManager;
    // std::unique_ptr<GridAxisHelper> gridAxisHelper;
    
    // 窗口尺寸（用于投影矩阵）
    int viewportWidth;
    int viewportHeight;
    
private:
    // 鼠标状态（用于相机控制）
    QPoint lastMousePos;
    bool isMousePressed;
};

#endif // DEMO_H