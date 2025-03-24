#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include <QPushButton>
#include <QComboBox>
#include "canvaswidget.h"

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
private:
    CanvasWidget *canvas;
    QPushButton *colorButton;
    QPushButton *clearButton;
    QPushButton *penWidthButton;
    QComboBox *modeComboBox;
    QComboBox *lineStyleComboBox;

    void setupUI();  // 初始化 UI
    void applyStyleSheet();  // 设置样式
};

#endif // MAINWINDOW_H
