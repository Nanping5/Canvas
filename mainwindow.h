//@author Nanping5
//@date 2025/3/24
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include <QPushButton>
#include <QComboBox>
#include "canvaswidget.h"
#include "animationwindow.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void selectColor();
    void clearCanvas();
    void setPenWidth();
    void setDrawingMode(int index);
    void setLineStyle(int index);
    void selectEraser();
    void saveCanvas();

private:
    CanvasWidget *canvas;
    QPushButton *colorButton;
    QPushButton *clearButton;
    QPushButton *penWidthButton;
    QComboBox *modeComboBox;
    QComboBox *lineStyleComboBox;
    QPushButton *eraserButton;
    QPushButton *playButton;
    AnimationWindow *animationWindow = nullptr;  // 新增成员变量

    void setupUI();  // 初始化 UI
    void applyStyleSheet();  // 设置样式
};

#endif // MAINWINDOW_H
