#ifndef GLWIDGET_H
#define GLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QTimer>
#include <QElapsedTimer>
#include <memory>
#include <functional>
#include <map>

// 前向声明
class Demo;
class QDockWidget;

/**
 * Demo 工厂函数类型
 * 用于创建 Demo 实例的函数指针
 */
using DemoFactory = std::function<std::unique_ptr<Demo>()>;

/**
 * Demo 注册信息
 */
struct DemoInfo {
    QString name;           // Demo 名称
    QString description;    // Demo 描述
    QString category;       // Demo 分类（如 "Basic", "Lighting", "Advanced"）
    DemoFactory factory;    // Demo 工厂函数
    
    DemoInfo() = default;
    DemoInfo(const QString &n, const QString &d, const QString &c, DemoFactory f)
        : name(n), description(d), category(c), factory(std::move(f)) {}
};

/**
 * GLWidget - OpenGL 渲染窗口部件
 * 
 * 负责：
 * 1. 管理 OpenGL 上下文
 * 2. 注册和管理多个 Demo
 * 3. 提供 Demo 选择界面
 * 4. 转发输入事件到当前 Demo
 * 5. 计算和显示 FPS
 */
class GLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit GLWidget(QWidget *parent = nullptr);
    ~GLWidget() override;

    // ============================================
    // Demo 注册系统
    // ============================================
    
    /**
     * 注册一个 Demo
     * @param id Demo 的唯一标识符
     * @param name Demo 显示名称
     * @param description Demo 描述
     * @param category Demo 分类
     * @param factory Demo 工厂函数
     * @return 是否注册成功
     */
    bool registerDemo(const QString &id, 
                     const QString &name,
                     const QString &description,
                     const QString &category,
                     DemoFactory factory);
    
    /**
     * 使用模板注册 Demo（更方便的方式）
     * @tparam T Demo 类型（必须继承自 Demo）
     * @param id Demo 唯一标识符
     * @param category Demo 分类
     */
    template<typename T>
    bool registerDemo(const QString &id, const QString &category = "General") {
        static_assert(std::is_base_of<Demo, T>::value, "T must inherit from Demo");
        
        // 创建一个临时实例以获取名称和描述
        auto tempDemo = std::make_unique<T>();
        QString name = tempDemo->getName();
        QString description = tempDemo->getDescription();
        tempDemo.reset();
        
        return registerDemo(id, name, description, category, []() {
            return std::make_unique<T>();
        });
    }
    
    /**
     * 取消注册 Demo
     * @param id Demo ID
     */
    void unregisterDemo(const QString &id);
    
    /**
     * 获取所有已注册的 Demo ID 列表
     */
    QStringList getRegisteredDemoIds() const;
    
    /**
     * 获取所有分类
     */
    QStringList getCategories() const;
    
    /**
     * 根据分类获取 Demo ID 列表
     */
    QStringList getDemosByCategory(const QString &category) const;
    
    /**
     * 获取 Demo 信息
     */
    const DemoInfo* getDemoInfo(const QString &id) const;

    // ============================================
    // Demo 管理
    // ============================================
    
    /**
     * 通过 ID 加载并切换到指定 Demo
     * @param id Demo ID
     * @return 是否成功加载
     */
    bool loadDemo(const QString &id);
    
    /**
     * 设置当前要显示的 Demo（直接设置实例）
     * @param demo Demo 的唯一指针（转移所有权）
     */
    void setDemo(std::unique_ptr<Demo> demo);
    
    /**
     * 清除当前 Demo
     */
    void clearDemo();
    
    /**
     * 获取当前 Demo（只读）
     */
    Demo* getCurrentDemo() const { return currentDemo.get(); }
    
    /**
     * 获取当前 Demo 的 ID
     */
    QString getCurrentDemoId() const { return currentDemoId; }
    
    /**
     * 检查是否有活动的 Demo
     */
    bool hasDemo() const { return currentDemo != nullptr; }

    // ============================================
    // UI 控制面板
    // ============================================
    
    /**
     * 创建 Demo 选择器停靠窗口
     * @param parent 父窗口
     * @return 停靠窗口指针（调用者负责管理）
     */
    QDockWidget* createDemoSelectorDock(QWidget *parent = nullptr);
    
    /**
     * 创建当前 Demo 的控制面板停靠窗口
     * @param parent 父窗口
     * @return 停靠窗口指针（调用者负责管理）
     */
    QDockWidget* createControlPanelDock(QWidget *parent = nullptr);

    // ============================================
    // 渲染控制
    // ============================================
    
    /**
     * 启动/停止自动渲染
     */
    void setAutoUpdate(bool enabled);
    
    /**
     * 获取自动更新状态
     */
    bool isAutoUpdate() const { return autoUpdate; }
    
    /**
     * 设置目标帧率（0 = 无限制）
     */
    void setTargetFPS(int fps);
    
    /**
     * 获取目标帧率
     */
    int getTargetFPS() const { return targetFPS; }
    
    /**
     * 获取当前 FPS
     */
    int getCurrentFPS() const { return lastFPS; }

signals:
    /**
     * FPS 更新信号（每秒发送一次）
     */
    void fpsUpdated(int fps);
    
    /**
     * 状态消息信号
     */
    void statusMessage(const QString &message);
    
    /**
     * Demo 切换信号
     */
    void demoChanged(Demo* newDemo, const QString &demoId);
    
    /**
     * Demo 注册/注销信号
     */
    void demoRegistered(const QString &id);
    void demoUnregistered(const QString &id);

protected:
    // ============================================
    // OpenGL 核心函数（Qt 框架回调）
    // ============================================
    
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    
    // ============================================
    // 输入事件处理（转发给 Demo）
    // ============================================
    
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    /**
     * 更新 FPS 计数器（每秒调用）
     */
    void updateFPS();
    
    /**
     * 处理 Demo 的状态消息
     */
    void onDemoStatusMessage(const QString &message);
    
    /**
     * 更新控制面板
     */
    void updateControlPanel();

private:
    // ============================================
    // 辅助方法
    // ============================================
    
    void printOpenGLInfo();
    void calculateDeltaTime();

    // ============================================
    // 成员变量
    // ============================================
    
    // Demo 注册表
    std::map<QString, DemoInfo> demoRegistry;
    
    // Demo 管理
    std::unique_ptr<Demo> currentDemo;
    std::unique_ptr<InputManager> input;
    QString currentDemoId;
    
    // UI 组件（弱引用，生命周期由外部管理）
    QDockWidget *controlPanelDock;
    
    // 渲染控制
    bool autoUpdate;
    int targetFPS;
    
    // FPS 计算
    QTimer *fpsTimer;
    QElapsedTimer frameTimer;
    int frameCount;
    int lastFPS;
    
    // 时间管理
    float deltaTime;
    float lastFrameTime;
    qint64 lastUpdateTime;
    
    // OpenGL 初始化标志
    bool glInitialized;
};

#endif // GLWIDGET_H