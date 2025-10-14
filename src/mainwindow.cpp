#include "MainWindow.h"
#include "base/opengl/glwidget.h"
#include "demo/triangle/TriangleDemo.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QStatusBar>
#include <QDockWidget>
#include <QLabel>
#include <QApplication>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 创建 GLWidget
    glWidget = new GLWidget(this);
    setCentralWidget(glWidget);
    
    // 注册 Demo
    registerDemos();
    
    // 创建菜单
    createMenus();
    
    // 创建停靠窗口
    createDockWindows();
    
    // 连接信号
    connectSignals();
    
    // 设置窗口
    setWindowTitle("OpenGL Demo Framework");
    resize(1280, 720);
    
    // 加载第一个 Demo
    glWidget->loadDemo("triangle");
}

MainWindow::~MainWindow()
{
}

void MainWindow::registerDemos()
{
    // 注册三角形 Demo
    glWidget->registerDemo<TriangleDemo>("triangle", "Basic");
    
    // 未来可以在这里注册更多 Demo
    // glWidget->registerDemo<CubeDemo>("cube", "Basic");
    // glWidget->registerDemo<LightingDemo>("lighting", "Lighting");
}

void MainWindow::createMenus()
{
    // 文件菜单
    QMenu *fileMenu = menuBar()->addMenu("&File");
    
    QAction *exitAction = new QAction("E&xit", this);
    exitAction->setShortcut(QKeySequence::Quit);
    exitAction->setStatusTip("Exit the application");
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    fileMenu->addAction(exitAction);
    
    // Demo 菜单
    QMenu *demoMenu = menuBar()->addMenu("&Demo");
    
    QAction *clearDemoAction = new QAction("&Clear Demo", this);
    clearDemoAction->setStatusTip("Clear current demo");
    connect(clearDemoAction, &QAction::triggered, glWidget, &GLWidget::clearDemo);
    demoMenu->addAction(clearDemoAction);
    
    demoMenu->addSeparator();
    
    // 动态创建 Demo 菜单项
    updateDemoMenu(demoMenu);
    
    // 视图菜单
    viewMenu = menuBar()->addMenu("&View");
    
    // 渲染菜单
    QMenu *renderMenu = menuBar()->addMenu("&Render");
    
    QAction *vsyncAction = new QAction("V-Sync (60 FPS)", this);
    vsyncAction->setCheckable(true);
    vsyncAction->setChecked(false);
    connect(vsyncAction, &QAction::toggled, [this](bool checked) {
        glWidget->setTargetFPS(checked ? 60 : 0);
        statusBar()->showMessage(checked ? "V-Sync enabled (60 FPS)" : "V-Sync disabled (unlimited FPS)", 2000);
    });
    renderMenu->addAction(vsyncAction);
    
    QAction *pauseAction = new QAction("&Pause Rendering", this);
    pauseAction->setCheckable(true);
    pauseAction->setShortcut(Qt::Key_Space);
    connect(pauseAction, &QAction::toggled, [this](bool checked) {
        glWidget->setAutoUpdate(!checked);
        statusBar()->showMessage(checked ? "Rendering paused" : "Rendering resumed", 2000);
    });
    renderMenu->addAction(pauseAction);
    
    // 帮助菜单
    QMenu *helpMenu = menuBar()->addMenu("&Help");
    
    QAction *aboutAction = new QAction("&About", this);
    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAbout);
    helpMenu->addAction(aboutAction);
}

void MainWindow::updateDemoMenu(QMenu *demoMenu)
{
    // 获取所有分类
    QStringList categories = glWidget->getCategories();
    categories.sort();
    
    for (const QString &category : categories) {
        QMenu *categoryMenu = demoMenu->addMenu(category);
        
        // 获取该分类下的所有 Demo
        QStringList demoIds = glWidget->getDemosByCategory(category);
        demoIds.sort();
        
        for (const QString &id : demoIds) {
            const DemoInfo *info = glWidget->getDemoInfo(id);
            if (info) {
                QAction *action = new QAction(info->name, this);
                action->setStatusTip(info->description);
                connect(action, &QAction::triggered, [this, id]() {
                    glWidget->loadDemo(id);
                });
                categoryMenu->addAction(action);
            }
        }
    }
}

void MainWindow::createDockWindows()
{
    // Demo 选择器停靠窗口
    QDockWidget *selectorDock = glWidget->createDemoSelectorDock(this);
    selectorDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::LeftDockWidgetArea, selectorDock);
    viewMenu->addAction(selectorDock->toggleViewAction());
    
    // 控制面板停靠窗口
    QDockWidget *controlDock = glWidget->createControlPanelDock(this);
    controlDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    addDockWidget(Qt::RightDockWidgetArea, controlDock);
    viewMenu->addAction(controlDock->toggleViewAction());
}

void MainWindow::connectSignals()
{
    // FPS 显示
    fpsLabel = new QLabel("FPS: 0");
    statusBar()->addPermanentWidget(fpsLabel);
    
    connect(glWidget, &GLWidget::fpsUpdated, [this](int fps) {
        fpsLabel->setText(QString("FPS: %1").arg(fps));
    });
    
    // 状态消息
    connect(glWidget, &GLWidget::statusMessage, [this](const QString &msg) {
        statusBar()->showMessage(msg, 3000);
    });
    
    // Demo 切换
    connect(glWidget, &GLWidget::demoChanged, [this](Demo *demo, const QString &id) {
        if (demo) {
            setWindowTitle(QString("OpenGL Demo Framework - %1").arg(demo->getName()));
            statusBar()->showMessage(QString("Loaded: %1").arg(demo->getName()), 3000);
        } else {
            setWindowTitle("OpenGL Demo Framework");
            statusBar()->showMessage("No demo loaded", 3000);
        }
    });
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "About OpenGL Demo Framework",
        "<h2>OpenGL Demo Framework</h2>"
        "<p>A flexible framework for creating and managing OpenGL demos.</p>"
        "<p><b>Features:</b></p>"
        "<ul>"
        "<li>Easy demo registration and management</li>"
        "<li>Built-in camera system</li>"
        "<li>Light management</li>"
        "<li>Interactive control panels</li>"
        "</ul>"
        "<p>Built with Qt and OpenGL 3.3+</p>"
    );
}