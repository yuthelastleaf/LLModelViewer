#include "GLWidget.h"
#include "../Demo.h"
#include <QMouseEvent>
#include <QKeyEvent>
#include <QWheelEvent>
#include <QDockWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QScrollArea>
#include <QSplitter>
#include <QDebug>

GLWidget::GLWidget(QWidget *parent)
    : QOpenGLWidget(parent)
    , currentDemo(nullptr)
    , input(std::make_unique<InputManager>())
    , controlPanelDock(nullptr)
    , autoUpdate(true)
    , targetFPS(0)
    , frameCount(0)
    , lastFPS(0)
    , deltaTime(0.0f)
    , lastFrameTime(0.0f)
    , lastUpdateTime(0)
    , glInitialized(false)
{
    // FPS 计时器（每秒更新一次）
    fpsTimer = new QTimer(this);
    connect(fpsTimer, &QTimer::timeout, this, &GLWidget::updateFPS);
    fpsTimer->start(1000);
    
    // 启动帧计时器
    frameTimer.start();
    
    // 设置焦点策略以接收键盘事件
    setFocusPolicy(Qt::StrongFocus);
    qDebug() << "GLWidget created";
}

GLWidget::~GLWidget()
{
    qDebug() << "GLWidget destroying...";
    
    makeCurrent();
    
    if (currentDemo) {
        qDebug() << "Cleaning up demo:" << currentDemo->getName();
        currentDemo->cleanup();
        currentDemo.reset();
    }
    
    doneCurrent();
    
    qDebug() << "GLWidget destroyed";
}

// ============================================
// Demo 注册系统
// ============================================

bool GLWidget::registerDemo(const QString &id,
                            const QString &name,
                            const QString &description,
                            const QString &category,
                            DemoFactory factory)
{
    if (id.isEmpty()) {
        qWarning() << "Cannot register demo with empty ID";
        return false;
    }
    
    if (demoRegistry.find(id) != demoRegistry.end()) {
        qWarning() << "Demo with ID" << id << "is already registered";
        return false;
    }
    
    if (!factory) {
        qWarning() << "Cannot register demo with null factory";
        return false;
    }
    
    DemoInfo info(name, description, category, std::move(factory));
    demoRegistry[id] = std::move(info);
    
    qDebug() << "Registered demo:" << id << "-" << name << "[" << category << "]";
    
    emit demoRegistered(id);
    return true;
}

void GLWidget::unregisterDemo(const QString &id)
{
    auto it = demoRegistry.find(id);
    if (it != demoRegistry.end()) {
        // 如果当前正在显示这个 Demo，先清除
        if (currentDemoId == id) {
            clearDemo();
        }
        
        qDebug() << "Unregistered demo:" << id;
        demoRegistry.erase(it);
        emit demoUnregistered(id);
    }
}

QStringList GLWidget::getRegisteredDemoIds() const
{
    QStringList ids;
    for (const auto &pair : demoRegistry) {
        ids.append(pair.first);
    }
    return ids;
}

QStringList GLWidget::getCategories() const
{
    QSet<QString> categories;
    for (const auto &pair : demoRegistry) {
        categories.insert(pair.second.category);
    }
    return categories.values();
}

QStringList GLWidget::getDemosByCategory(const QString &category) const
{
    QStringList demos;
    for (const auto &pair : demoRegistry) {
        if (pair.second.category == category) {
            demos.append(pair.first);
        }
    }
    return demos;
}

const DemoInfo* GLWidget::getDemoInfo(const QString &id) const
{
    auto it = demoRegistry.find(id);
    return (it != demoRegistry.end()) ? &it->second : nullptr;
}

// ============================================
// Demo 管理
// ============================================

bool GLWidget::loadDemo(const QString &id)
{
    auto it = demoRegistry.find(id);
    if (it == demoRegistry.end()) {
        qWarning() << "Demo not found:" << id;
        emit statusMessage(QString("Error: Demo '%1' not found").arg(id));
        return false;
    }
    
    try {
        // 使用工厂函数创建 Demo
        auto demo = it->second.factory();
        if (!demo) {
            qWarning() << "Failed to create demo:" << id;
            emit statusMessage(QString("Error: Failed to create demo '%1'").arg(id));
            return false;
        }
        
        // 设置 Demo
        currentDemoId = id;
        setDemo(std::move(demo));
        
        qDebug() << "Loaded demo:" << id;
        return true;
        
    } catch (const std::exception &e) {
        qCritical() << "Exception while loading demo" << id << ":" << e.what();
        emit statusMessage(QString("Error: Exception while loading demo: %1").arg(e.what()));
        return false;
    }
}

void GLWidget::setDemo(std::unique_ptr<Demo> demo)
{
    makeCurrent();
    
    // 清理旧的 Demo
    if (currentDemo) {
        qDebug() << "Cleaning up old demo:" << currentDemo->getName();
        
        // 断开旧 Demo 的信号
        disconnect(currentDemo.get(), nullptr, this, nullptr);
        
        currentDemo->cleanup();
    }
    
    // 设置新的 Demo
    currentDemo = std::move(demo);
    
    if (currentDemo) {
        qDebug() << "Setting up new demo:" << currentDemo->getName();
        
        // 只有在 OpenGL 已初始化的情况下才初始化 Demo
        if (glInitialized) {
            currentDemo->initialize();
        }
        
        // 设置视口尺寸
        currentDemo->resizeViewport(width(), height());
        
        // 连接 Demo 的信号
        connect(currentDemo.get(), &Demo::statusMessage,
                this, &GLWidget::onDemoStatusMessage);
        connect(currentDemo.get(), &Demo::parameterChanged,
                this, &GLWidget::updateControlPanel);
        connect(currentDemo.get(), &Demo::parameterChanged,
                this, [this]() { update(); });
        
        emit statusMessage(QString("Demo loaded: %1").arg(currentDemo->getName()));
    } else {
        qDebug() << "Demo cleared";
        currentDemoId.clear();
        emit statusMessage("No demo loaded");
    }
    
    doneCurrent();
    
    emit demoChanged(currentDemo.get(), currentDemoId);
    
    // 更新控制面板
    updateControlPanel();
    
    update();
}

void GLWidget::clearDemo()
{
    currentDemoId.clear();
    setDemo(nullptr);
}

// ============================================
// UI 控制面板
// ============================================

QDockWidget* GLWidget::createDemoSelectorDock(QWidget *parent)
{
    QDockWidget *dock = new QDockWidget("Demo Selector", parent);
    dock->setObjectName("DemoSelectorDock");
    
    QWidget *container = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(container);
    
    // 分类选择器
    QHBoxLayout *categoryLayout = new QHBoxLayout();
    categoryLayout->addWidget(new QLabel("Category:"));
    QComboBox *categoryCombo = new QComboBox();
    categoryCombo->addItem("All");
    QStringList categories = getCategories();
    categories.sort();
    categoryCombo->addItems(categories);
    categoryLayout->addWidget(categoryCombo, 1);
    layout->addLayout(categoryLayout);
    
    // Demo 列表
    QListWidget *demoList = new QListWidget();
    demoList->setSelectionMode(QAbstractItemView::SingleSelection);
    layout->addWidget(demoList);
    
    // 描述标签
    QLabel *descriptionLabel = new QLabel("Select a demo to see its description");
    descriptionLabel->setWordWrap(true);
    descriptionLabel->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    descriptionLabel->setMinimumHeight(60);
    descriptionLabel->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    descriptionLabel->setMargin(5);
    layout->addWidget(descriptionLabel);
    
    // 加载按钮
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    QPushButton *loadButton = new QPushButton("Load Demo");
    QPushButton *clearButton = new QPushButton("Clear");
    loadButton->setEnabled(false);
    buttonLayout->addWidget(loadButton);
    buttonLayout->addWidget(clearButton);
    layout->addLayout(buttonLayout);
    
    // 填充 Demo 列表的函数
    auto populateDemoList = [this, demoList, categoryCombo]() {
        demoList->clear();
        
        QString selectedCategory = categoryCombo->currentText();
        QStringList demoIds;
        
        if (selectedCategory == "All") {
            demoIds = getRegisteredDemoIds();
        } else {
            demoIds = getDemosByCategory(selectedCategory);
        }
        
        demoIds.sort();
        
        for (const QString &id : demoIds) {
            const DemoInfo *info = getDemoInfo(id);
            if (info) {
                QListWidgetItem *item = new QListWidgetItem(info->name);
                item->setData(Qt::UserRole, id);
                
                // 如果是当前 Demo，高亮显示
                if (id == currentDemoId) {
                    QFont font = item->font();
                    font.setBold(true);
                    item->setFont(font);
                    item->setBackground(QColor(220, 240, 255));
                }
                
                demoList->addItem(item);
            }
        }
    };
    
    // 初始填充
    populateDemoList();
    
    // 连接信号
    connect(categoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            populateDemoList);
    
    connect(demoList, &QListWidget::currentItemChanged,
            [this, descriptionLabel, loadButton](QListWidgetItem *current, QListWidgetItem *) {
        if (current) {
            QString id = current->data(Qt::UserRole).toString();
            const DemoInfo *info = getDemoInfo(id);
            if (info) {
                QString desc = QString("<b>%1</b><br><br>%2<br><br><i>Category: %3</i>")
                    .arg(info->name)
                    .arg(info->description.isEmpty() ? "No description available." : info->description)
                    .arg(info->category);
                descriptionLabel->setText(desc);
            }
            loadButton->setEnabled(true);
        } else {
            descriptionLabel->setText("Select a demo to see its description");
            loadButton->setEnabled(false);
        }
    });
    
    connect(demoList, &QListWidget::itemDoubleClicked,
            [this](QListWidgetItem *item) {
        QString id = item->data(Qt::UserRole).toString();
        loadDemo(id);
    });
    
    connect(loadButton, &QPushButton::clicked,
            [this, demoList]() {
        QListWidgetItem *item = demoList->currentItem();
        if (item) {
            QString id = item->data(Qt::UserRole).toString();
            loadDemo(id);
        }
    });
    
    connect(clearButton, &QPushButton::clicked,
            this, &GLWidget::clearDemo);
    
    // 当 Demo 改变时，更新列表
    connect(this, &GLWidget::demoChanged,
            [populateDemoList](Demo*, const QString&) {
        populateDemoList();
    });
    
    // 当注册/注销 Demo 时，更新列表
    connect(this, &GLWidget::demoRegistered,
            [populateDemoList, categoryCombo](const QString&) {
        // 更新分类列表
        QString current = categoryCombo->currentText();
        categoryCombo->blockSignals(true);
        categoryCombo->clear();
        categoryCombo->addItem("All");
        // 需要重新获取分类...这里简化处理
        categoryCombo->blockSignals(false);
        populateDemoList();
    });
    
    connect(this, &GLWidget::demoUnregistered,
            populateDemoList);
    
    dock->setWidget(container);
    return dock;
}

QDockWidget* GLWidget::createControlPanelDock(QWidget *parent)
{
    QDockWidget *dock = new QDockWidget("Control Panel", parent);
    dock->setObjectName("ControlPanelDock");
    
    // 保存弱引用
    controlPanelDock = dock;
    
    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    
    // 初始内容
    QWidget *placeholder = new QWidget();
    QVBoxLayout *placeholderLayout = new QVBoxLayout(placeholder);
    placeholderLayout->addWidget(new QLabel("No demo loaded"));
    placeholderLayout->addStretch();
    
    scrollArea->setWidget(placeholder);
    dock->setWidget(scrollArea);
    
    // 当 Demo 改变时，更新控制面板
    connect(this, &GLWidget::demoChanged,
            [this, scrollArea](Demo* demo, const QString&) {
        if (demo) {
            QWidget *controlPanel = demo->createControlPanel();
            if (controlPanel) {
                scrollArea->setWidget(controlPanel);
            }
        } else {
            QWidget *placeholder = new QWidget();
            QVBoxLayout *layout = new QVBoxLayout(placeholder);
            layout->addWidget(new QLabel("No demo loaded"));
            layout->addStretch();
            scrollArea->setWidget(placeholder);
        }
    });
    
    return dock;
}

// ============================================
// 渲染控制
// ============================================

void GLWidget::setAutoUpdate(bool enabled)
{
    autoUpdate = enabled;
    
    if (autoUpdate) {
        update();
    }
}

void GLWidget::setTargetFPS(int fps)
{
    targetFPS = fps;
}

// ============================================
// OpenGL 核心函数
// ============================================

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    
    // if (!gladLoadGL()) {
    //     qCritical() << "Failed to initialize GLAD!";
    //     emit statusMessage("ERROR: Failed to initialize GLAD");
    //     return;
    // }
    
    glInitialized = true;
    
    printOpenGLInfo();
    
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glEnable(GL_MULTISAMPLE);
    
    if (currentDemo) {
        qDebug() << "Initializing demo:" << currentDemo->getName();
        currentDemo->initialize();
        emit statusMessage(QString("Demo initialized: %1").arg(currentDemo->getName()));
    }
    
    emit statusMessage("OpenGL initialized successfully");
}

void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    
    if (currentDemo) {
        currentDemo->resizeViewport(w, h);
    }
}

void GLWidget::paintGL()
{~
    input->beginFrame();
    calculateDeltaTime();
    
    
    if (targetFPS > 0) {
        qint64 currentTime = frameTimer.elapsed();
        qint64 targetFrameTime = 1000 / targetFPS;
        qint64 elapsedSinceLastUpdate = currentTime - lastUpdateTime;
        
        if (elapsedSinceLastUpdate < targetFrameTime) {
            update();
            return;
        }
        
        lastUpdateTime = currentTime;
    }

    if (currentDemo) {
        if (input->isKeyDown(Qt::Key_W)) currentDemo->camera.processKeyboard(CameraMovement::FORWARD,  deltaTime);
        if (input->isKeyDown(Qt::Key_S)) currentDemo->camera.processKeyboard(CameraMovement::BACKWARD, deltaTime);
        if (input->isKeyDown(Qt::Key_A)) currentDemo->camera.processKeyboard(CameraMovement::LEFT,     deltaTime);
        if (input->isKeyDown(Qt::Key_D)) currentDemo->camera.processKeyboard(CameraMovement::RIGHT,    deltaTime);
        if (input->isKeyDown(Qt::Key_E)) currentDemo->camera.processKeyboard(CameraMovement::UP,       deltaTime);
        if (input->isKeyDown(Qt::Key_Q)) currentDemo->camera.processKeyboard(CameraMovement::DOWN,     deltaTime);
    }
    
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    if (currentDemo) {
        currentDemo->update(deltaTime);
        currentDemo->render();
        
        GLenum err;
        while ((err = glGetError()) != GL_NO_ERROR) {
            qWarning() << "OpenGL error:" << err;
        }
    }
    
    frameCount++;
    
    if (autoUpdate) {
        update();
    }
}

// ============================================
// 输入事件处理
// ============================================

void GLWidget::keyPressEvent(QKeyEvent *event)
{
    if (input) {
        input->onKeyPress(event);
    }
    QOpenGLWidget::keyPressEvent(event);
}

void GLWidget::keyReleaseEvent(QKeyEvent *event)
{
    if (input) {
        input->onKeyRelease(event);
    }
    QOpenGLWidget::keyReleaseEvent(event);
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    setFocus();
    if(input) {
        input->onMousePress(event);
    }

    QOpenGLWidget::mousePressEvent(event);
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    if(input) {
        input->onMouseMove(event);
    }
    QOpenGLWidget::mouseMoveEvent(event);
}

void GLWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if(input) {
        input->onMouseRelease(event);
    }
    QOpenGLWidget::mouseReleaseEvent(event);
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
    if(input) {
        input->onWheel(event);
    }
    QOpenGLWidget::wheelEvent(event);
}

// ============================================
// 私有槽函数
// ============================================

void GLWidget::updateFPS()
{
    lastFPS = frameCount;
    frameCount = 0;
    emit fpsUpdated(lastFPS);
}

void GLWidget::onDemoStatusMessage(const QString &message)
{
    emit statusMessage(message);
}

void GLWidget::updateControlPanel()
{
    // 触发重新创建控制面板
    if (controlPanelDock && currentDemo) {
        QScrollArea *scrollArea = qobject_cast<QScrollArea*>(controlPanelDock->widget());
        if (scrollArea) {
            QWidget *oldWidget = scrollArea->takeWidget();
            if (oldWidget) {
                oldWidget->deleteLater();
            }
            
            QWidget *newPanel = currentDemo->createControlPanel();
            if (newPanel) {
                scrollArea->setWidget(newPanel);
            }
        }
    }
}

// ============================================
// 辅助方法
// ============================================

void GLWidget::printOpenGLInfo()
{
    qDebug() << "========================================";
    qDebug() << "OpenGL Information:";
    qDebug() << "========================================";
    qDebug() << "Version:   " << (const char*)glGetString(GL_VERSION);
    qDebug() << "GLSL:      " << (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    qDebug() << "Vendor:    " << (const char*)glGetString(GL_VENDOR);
    qDebug() << "Renderer:  " << (const char*)glGetString(GL_RENDERER);
    
    GLint maxTextureSize, maxTextureUnits, maxVertexAttribs;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);
    glGetIntegerv(GL_MAX_TEXTURE_IMAGE_UNITS, &maxTextureUnits);
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &maxVertexAttribs);
    
    qDebug() << "----------------------------------------";
    qDebug() << "Capabilities:";
    qDebug() << "  Max Texture Size:   " << maxTextureSize;
    qDebug() << "  Max Texture Units:  " << maxTextureUnits;
    qDebug() << "  Max Vertex Attribs: " << maxVertexAttribs;
    
    GLint samples;
    glGetIntegerv(GL_SAMPLES, &samples);
    qDebug() << "  MSAA Samples:       " << samples;
    
    qDebug() << "========================================";
}

void GLWidget::calculateDeltaTime()
{
    float currentFrameTime = frameTimer.elapsed() / 1000.0f;
    deltaTime = currentFrameTime - lastFrameTime;
    
    if (deltaTime > 0.1f) {
        deltaTime = 0.016f;
    }
    
    lastFrameTime = currentFrameTime;
}