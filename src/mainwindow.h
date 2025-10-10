#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include <QMenuBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QTreeWidget>
#include <QSlider>
#include <QLabel>

class GLWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onOpenFile();
    void onSaveFile();
    void onResetView();
    void onShowWireframe(bool checked);
    void onUpdateFPS(int fps);

private:
    void setupUI();
    void createMenuBar();
    void createToolBar();
    void createDockWidgets();
    void createStatusBar();

    GLWidget *glWidget;
    QToolBar *mainToolBar;
    QDockWidget *propertyDock;
    QDockWidget *sceneDock;
    QTreeWidget *sceneTree;
    QLabel *fpsLabel;
};

#endif // MAINWINDOW_H