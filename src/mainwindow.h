#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QDockWidget>

class GLWidget;
class Demo;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Demo 类型枚举
    enum class DemoType {
        Triangle,
        Texture,
        Model
    };

private slots:
    void onUpdateFPS(int fps);
    void onDemoChanged(Demo *demo);

private:
    void setupUI();
    void createMenuBar();
    void createToolBar();
    void createDockWidgets();
    void createStatusBar();
    
    // Demo 管理
    void loadDemo(DemoType type);
    void updateControlPanel(Demo *demo);

    // UI 组件
    GLWidget *glWidget;
    QDockWidget *sceneDock;
    QDockWidget *controlDock;
    QLabel *fpsLabel;
};

#endif // MAINWINDOW_H