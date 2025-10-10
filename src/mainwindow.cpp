#include "mainwindow.h"
#include "glwidget.h"
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QSlider>
#include <QPushButton>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    
    setWindowTitle("OpenGL 模型查看器");
    resize(1280, 720);
}

MainWindow::~MainWindow()
{
}

void MainWindow::setupUI()
{
    // 创建中央OpenGL窗口
    glWidget = new GLWidget(this);
    setCentralWidget(glWidget);
    
    // ⚠️ 注意：先创建停靠窗口，再创建菜单栏！
    createDockWidgets();
    
    // 创建菜单栏（现在可以安全引用 propertyDock 和 sceneDock）
    createMenuBar();
    
    // 创建工具栏
    createToolBar();
    
    // 创建状态栏
    createStatusBar();
    
    // 连接信号
    connect(glWidget, &GLWidget::fpsUpdated, this, &MainWindow::onUpdateFPS);
}

void MainWindow::createMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu("文件(&F)");
    
    QAction *openAction = fileMenu->addAction("打开模型(&O)...");
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);
    
    QAction *saveAction = fileMenu->addAction("保存场景(&S)...");
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveFile);
    
    fileMenu->addSeparator();
    
    QAction *exitAction = fileMenu->addAction("退出(&X)");
    exitAction->setShortcut(QKeySequence::Quit);
    connect(exitAction, &QAction::triggered, this, &QWidget::close);
    
    QMenu *viewMenu = menuBar()->addMenu("视图(&V)");
    
    QAction *resetViewAction = viewMenu->addAction("重置视图(&R)");
    resetViewAction->setShortcut(Qt::Key_Home);
    connect(resetViewAction, &QAction::triggered, this, &MainWindow::onResetView);
    
    QAction *wireframeAction = viewMenu->addAction("线框模式(&W)");
    wireframeAction->setCheckable(true);
    connect(wireframeAction, &QAction::toggled, this, &MainWindow::onShowWireframe);
    
    viewMenu->addSeparator();
    viewMenu->addAction(propertyDock->toggleViewAction());
    viewMenu->addAction(sceneDock->toggleViewAction());
    
    QMenu *helpMenu = menuBar()->addMenu("帮助(&H)");
    QAction *aboutAction = helpMenu->addAction("关于(&A)");
    connect(aboutAction, &QAction::triggered, [this]() {
        QMessageBox::about(this, "关于", "OpenGL 模型查看器\n基于Qt和OpenGL开发");
    });
}

void MainWindow::createToolBar()
{
    mainToolBar = addToolBar("主工具栏");
    mainToolBar->setMovable(false);
    
    QAction *openAction = mainToolBar->addAction("打开");
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpenFile);
    
    QAction *saveAction = mainToolBar->addAction("保存");
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveFile);
    
    mainToolBar->addSeparator();
    
    QAction *resetAction = mainToolBar->addAction("重置");
    connect(resetAction, &QAction::triggered, this, &MainWindow::onResetView);
    
    QAction *wireframeAction = mainToolBar->addAction("线框");
    wireframeAction->setCheckable(true);
    connect(wireframeAction, &QAction::toggled, this, &MainWindow::onShowWireframe);
}

void MainWindow::createDockWidgets()
{
    // 场景树停靠窗口
    sceneDock = new QDockWidget("场景树", this);
    sceneDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    
    sceneTree = new QTreeWidget();
    sceneTree->setHeaderLabel("场景对象");
    
    // 添加示例节点
    QTreeWidgetItem *rootItem = new QTreeWidgetItem(sceneTree);
    rootItem->setText(0, "场景根节点");
    rootItem->setExpanded(true);
    
    sceneDock->setWidget(sceneTree);
    addDockWidget(Qt::LeftDockWidgetArea, sceneDock);
    
    // 属性面板停靠窗口
    propertyDock = new QDockWidget("属性面板", this);
    propertyDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    
    QWidget *propertyWidget = new QWidget();
    QVBoxLayout *propertyLayout = new QVBoxLayout(propertyWidget);
    
    // 相机控制组
    QGroupBox *cameraGroup = new QGroupBox("相机控制");
    QVBoxLayout *cameraLayout = new QVBoxLayout();
    
    cameraLayout->addWidget(new QLabel("距离:"));
    QSlider *distanceSlider = new QSlider(Qt::Horizontal);
    distanceSlider->setRange(1, 100);
    distanceSlider->setValue(50);
    cameraLayout->addWidget(distanceSlider);
    
    cameraLayout->addWidget(new QLabel("视场角:"));
    QSlider *fovSlider = new QSlider(Qt::Horizontal);
    fovSlider->setRange(30, 120);
    fovSlider->setValue(45);
    cameraLayout->addWidget(fovSlider);
    
    cameraGroup->setLayout(cameraLayout);
    propertyLayout->addWidget(cameraGroup);
    
    // 渲染设置组
    QGroupBox *renderGroup = new QGroupBox("渲染设置");
    QVBoxLayout *renderLayout = new QVBoxLayout();
    
    QPushButton *bgColorBtn = new QPushButton("背景颜色");
    renderLayout->addWidget(bgColorBtn);
    
    renderGroup->setLayout(renderLayout);
    propertyLayout->addWidget(renderGroup);
    
    propertyLayout->addStretch();
    
    propertyDock->setWidget(propertyWidget);
    addDockWidget(Qt::RightDockWidgetArea, propertyDock);
}

void MainWindow::createStatusBar()
{
    fpsLabel = new QLabel("FPS: 0");
    fpsLabel->setMinimumWidth(80);
    statusBar()->addPermanentWidget(fpsLabel);
    
    statusBar()->showMessage("就绪");
}

void MainWindow::onOpenFile()
{
    QString fileName = QFileDialog::getOpenFileName(
        this, "打开模型文件", "", 
        "模型文件 (*.obj *.fbx *.gltf);;所有文件 (*.*)");
    
    if (!fileName.isEmpty()) {
        statusBar()->showMessage("加载文件: " + fileName);
        // TODO: 调用GLWidget的加载模型接口
        // glWidget->loadModel(fileName);
    }
}

void MainWindow::onSaveFile()
{
    QString fileName = QFileDialog::getSaveFileName(
        this, "保存场景", "", "场景文件 (*.scene)");
    
    if (!fileName.isEmpty()) {
        statusBar()->showMessage("保存场景: " + fileName);
        // TODO: 实现场景保存功能
    }
}

void MainWindow::onResetView()
{
    glWidget->resetView();
    statusBar()->showMessage("视图已重置");
}

void MainWindow::onShowWireframe(bool checked)
{
    glWidget->setWireframeMode(checked);
    statusBar()->showMessage(checked ? "线框模式: 开" : "线框模式: 关");
}

void MainWindow::onUpdateFPS(int fps)
{
    fpsLabel->setText(QString("FPS: %1").arg(fps));
}