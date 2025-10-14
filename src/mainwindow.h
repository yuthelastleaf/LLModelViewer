#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>

class GLWidget;
class QMenu;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void registerDemos();
    void createMenus();
    void updateDemoMenu(QMenu *demoMenu);
    void createDockWindows();
    void connectSignals();
    void showAbout();

private:
    GLWidget *glWidget;
    QMenu *viewMenu;
    QLabel *fpsLabel;
};

#endif // MAINWINDOW_H