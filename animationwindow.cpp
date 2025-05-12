#include "animationwindow.h"
#include <QVBoxLayout>  // 添加垂直布局头文件

AnimationWindow::AnimationWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("动画窗口");
    setMinimumSize(1440, 960);  // 设置最小大小，但允许用户调整窗口大小
    
    backButton = new QPushButton("返回主界面", this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(backButton);
    layout->addStretch();  // 添加伸缩项，使按钮保持在顶部
    setLayout(layout);
    
    connect(backButton, &QPushButton::clicked, [this](){
        emit backToMain();
        close();
    });
} 