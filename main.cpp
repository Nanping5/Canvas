//@author Nanping5
//@date 2025/3/24
#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    a.setWindowIcon(QIcon(":/icons/icon.png"));

    MainWindow w;
    w.show();
    return a.exec();
}
