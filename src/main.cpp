#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 创建主窗口
    QWidget window;
    window.setWindowTitle("My First Qt CMake App");
    window.resize(400, 300);

    // 创建布局
    QVBoxLayout *layout = new QVBoxLayout(&window);

    // 添加标签
    QLabel *label = new QLabel("Hello, Qt with CMake!", &window);
    label->setAlignment(Qt::AlignCenter);
    layout->addWidget(label);

    // 添加按钮
    QPushButton *button = new QPushButton("Click Me!", &window);
    layout->addWidget(button);

    // 连接信号和槽
    QObject::connect(button, &QPushButton::clicked, [&]() {
        QMessageBox::information(&window, "Message", "Button clicked!");
        label->setText("Button was clicked!");
    });

    window.show();
    return app.exec();
}