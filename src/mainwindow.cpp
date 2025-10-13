#include "mainwindow.h"
#include "glwidget.h"
#include "Demo.h"
#include "TriangleDemo.h"
// #include "TextureDemo.h"
// #include "ModelDemo.h"

#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QScrollArea>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    
    setWindowTitle("OpenGL Demo Framework - Qt");
    resize(1280, 720);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    // 创建中央 OpenGL 窗口
    glWidget = new GLWidget(this);
    setCentralWidget(glWidget);
    
    // 创建停靠窗口
    createDockWidgets();
    
    // 创建菜单栏
    createMenuBar();
    
    // 创建工具栏
    createToolBar();
    
    // 创建状态栏
    createStatusBar();
    
    // 连接信号
    connect(glWidget, &GLWidget::fpsUpdated, this, &MainWindow::onUpdateFPS);
    connect(glWidget, &GLWidget::demoChanged, this, &MainWindow::onDemoChanged);
}

void MainWindow::createMenuBar()
{
    // ============================================
    // 文件菜单
    // ============================================
    QMenu *fileMenu = menuBar()->addMenu("文件(&F)");
    
    QAction *exitAction = fileMenu->addAction("退出(&X)");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    // ============================================
    // Demo 菜单
    // ============================================
    QMenu *demoMenu = menuBar()->addMenu("Demos(&D)");
    
    QAction *triangleDemoAction = demoMenu->addAction("Triangle Demo");
    connect(triangleDemoAction, &QAction::triggered, [this]() {
        loadDemo(DemoType::Triangle);
    });
    
    QAction *textureDemoAction = demoMenu->addAction("Texture Demo");
    connect(textureDemoAction, &QAction::triggered, [this]() {
        loadDemo(DemoType::Texture);
    });
    
    QAction *modelDemoAction = demoMenu->addAction("Model Demo");
    connect(modelDemoAction, &QAction::triggered, [this]() {
        loadDemo(DemoType::Model);
    });
    
    demoMenu->addSeparator();
    
    QAction *clearDemoAction = demoMenu->addAction("Clear Demo");
    connect(clearDemoAction, &QAction::triggered, [this]() {
        glWidget->clearDemo();
        updateControlPanel(nullptr);
        statusBar()->showMessage("Demo cleared");
    });
    
    // ============================================
    // 视图菜单
    // ============================================
    QMenu *viewMenu = menuBar()->addMenu("视图(&V)");
    
    viewMenu->addAction(sceneDock->toggleViewAction());
    viewMenu->addAction(controlDock->toggleViewAction());
    
    // ============================================
    // 帮助菜单
    // ============================================
    QMenu *helpMenu = menuBar()->addMenu("帮助(&H)");
    QAction *aboutAction = helpMenu->addAction("关于(&A)");
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "关于", 
            "OpenGL Demo Framework (Qt)\n\n"
            "基于 Qt + OpenGL 的演示框架\n"
            "支持多个可切换的 Demo\n\n"
            "控制说明:\n"
            "  WASD - 移动相机\n"
            "  鼠标拖动 - 旋转视角\n"
            "  滚轮 - 缩放\n"
            "  R - 重置相机");
    });
}

void MainWindow::createToolBar()
{
    QToolBar *mainToolBar = addToolBar("主工具栏");
    mainToolBar->setMovable(false);
    
    // Demo 快速切换按钮
    QAction *triangleAction = mainToolBar->addAction("Triangle");
    connect(triangleAction, &QAction::triggered, [this]() {
        loadDemo(DemoType::Triangle);
    });
    
    QAction *textureAction = mainToolBar->addAction("Texture");
    connect(textureAction, &QAction::triggered, [this]() {
        loadDemo(DemoType::Texture);
    });
    
    QAction *modelAction = mainToolBar->addAction("Model");
    connect(modelAction, &QAction::triggered, [this]() {
        loadDemo(DemoType::Model);
    });
    
    mainToolBar->addSeparator();
    
    QAction *clearAction = mainToolBar->addAction("Clear");
    connect(clearAction, &QAction::triggered, [this]() {
        glWidget->clearDemo();
        updateControlPanel(nullptr);
    });
}

void MainWindow::createDockWidgets()
{
    // ============================================
    // 场景树停靠窗口（左侧）
    // ============================================
    sceneDock = new QDockWidget("Scene", this);
    sceneDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    
    QTreeWidget *sceneTree = new QTreeWidget();
    sceneTree->setHeaderLabel("Scene Objects");
    
    QTreeWidgetItem *rootItem = new QTreeWidgetItem(sceneTree);
    rootItem->setText(0, "Scene Root");
    rootItem->setExpanded(true);
    
    sceneDock->setWidget(sceneTree);
    addDockWidget(Qt::LeftDockWidgetArea, sceneDock);
    
    // ============================================
    // 控制面板停靠窗口（右侧）
    // ============================================
    controlDock = new QDockWidget("Controls", this);
    controlDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    
    // 使用滚动区域包装控制面板
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    
    // 默认控制面板（无 Demo 时显示）
    QWidget *defaultPanel = new QWidget();
    QVBoxLayout *defaultLayout = new QVBoxLayout(defaultPanel);
    defaultLayout->addWidget(new QLabel("No Demo loaded.\nSelect a demo from the menu."));
    defaultLayout->addStretch();
    
    scrollArea->setWidget(defaultPanel);
    controlDock->setWidget(scrollArea);
    addDockWidget(Qt::RightDockWidgetArea, controlDock);
}

void MainWindow::createStatusBar()
{
    fpsLabel = new QLabel("FPS: 0");
    fpsLabel->setMinimumWidth(80);
    statusBar()->addPermanentWidget(fpsLabel);
    
    statusBar()->showMessage("就绪 - 请选择一个 Demo");
}

// ============================================
// Demo 管理
// ============================================

void MainWindow::loadDemo(DemoType type)
{
    std::unique_ptr<Demo> demo;
    
    switch (type) {
        case DemoType::Triangle:
            demo = std::make_unique<TriangleDemo>();
            break;
            
        case DemoType::Texture:
            // demo = std::make_unique<TextureDemo>();
            statusBar()->showMessage("Texture Demo - 待实现");
            return;
            
        case DemoType::Model:
            // demo = std::make_unique<ModelDemo>();
            statusBar()->showMessage("Model Demo - 待实现");
            return;
            
        default:
            return;
    }
    
    if (demo) {
        // 连接 Demo 的状态消息信号
        connect(demo.get(), &Demo::statusMessage, 
                statusBar(), &QStatusBar::showMessage);
        
        // 连接参数变化信号（用于触发重绘）
        connect(demo.get(), &Demo::parameterChanged,
                glWidget, QOverload<>::of(&QWidget::update));
        
        // 加载 Demo
        glWidget->setDemo(std::move(demo));
    }
}

void MainWindow::onDemoChanged(Demo *demo)
{
    updateControlPanel(demo);
    
    if (demo) {
        statusBar()->showMessage("Loaded: " + demo->getName());
    } else {
        statusBar()->showMessage("No demo loaded");
    }
}

void MainWindow::updateControlPanel(Demo *demo)
{
    QScrollArea *scrollArea = qobject_cast<QScrollArea*>(controlDock->widget());
    if (!scrollArea) return;
    
    // 删除旧的控制面板
    QWidget *oldPanel = scrollArea->widget();
    if (oldPanel) {
        scrollArea->takeWidget();
        oldPanel->deleteLater();
    }
    
    // 创建新的控制面板
    QWidget *newPanel = nullptr;
    
    if (demo) {
        newPanel = demo->createControlPanel();
    } else {
        // 默认面板
        newPanel = new QWidget();
        QVBoxLayout *layout = new QVBoxLayout(newPanel);
        layout->addWidget(new QLabel("No Demo loaded.\nSelect a demo from the menu."));
        layout->addStretch();
    }
    
    scrollArea->setWidget(newPanel);
}

void MainWindow::onUpdateFPS(int fps)
{
    fpsLabel->setText(QString("FPS: %1").arg(fps));
}