#include "MainWindow.h"
#include <QApplication>
#include <QSurfaceFormat>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // 设置应用程序信息
    app.setApplicationName("OpenGL Demo Framework");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("YourOrganization");
    
    // 设置 OpenGL 格式
    QSurfaceFormat format;
    format.setVersion(3, 3);                    // OpenGL 3.3
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSamples(4);                       // 4x MSAA 抗锯齿
    format.setDepthBufferSize(24);              // 24 位深度缓冲
    format.setStencilBufferSize(8);             // 8 位模板缓冲
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    
    // 设置为默认格式
    QSurfaceFormat::setDefaultFormat(format);
    
    qDebug() << "========================================";
    qDebug() << "OpenGL Demo Framework";
    qDebug() << "========================================";
    qDebug() << "Requested OpenGL version: 3.3 Core";
    qDebug() << "MSAA samples:" << format.samples();
    qDebug() << "========================================";
    
    // 创建并显示主窗口
    MainWindow window;
    window.show();
    
    qDebug() << "Application started successfully";
    
    return app.exec();
}