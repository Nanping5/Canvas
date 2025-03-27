#include "mainwindow.h"
#include <QColorDialog>
#include <QInputDialog>
#include <QVBoxLayout>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setupUI();
    applyStyleSheet();
}

MainWindow::~MainWindow() {}

void MainWindow::setupUI() {
    // 设置窗口
    setWindowTitle("Canvas");
    resize(1440,960);

    // 创建画布
    canvas = new CanvasWidget(this);
    setCentralWidget(canvas);

    // 创建工具栏
    QToolBar *toolBar = new QToolBar("工具栏", this);
    addToolBar(Qt::TopToolBarArea, toolBar);

    // 颜色选择按钮
    colorButton = new QPushButton("🎨 颜色", this);
    connect(colorButton, &QPushButton::clicked, this, &MainWindow::selectColor);

    // 清除按钮
    clearButton = new QPushButton("🗑 清除", this);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearCanvas);

    // 画笔粗细按钮
    penWidthButton = new QPushButton("✏️ 粗细", this);
    connect(penWidthButton, &QPushButton::clicked, this, &MainWindow::setPenWidth);

    // 选择绘制模式
    modeComboBox = new QComboBox(this);
    modeComboBox->addItem("自由绘制");
    modeComboBox->addItem("直线");
    modeComboBox->addItem("圆");
    connect(modeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::setDrawingMode);

    // 线型选择框
    lineStyleComboBox = new QComboBox(this);
    lineStyleComboBox->addItem("实线", QVariant::fromValue(Qt::SolidLine));
    lineStyleComboBox->addItem("虚线", QVariant::fromValue(Qt::DashLine));
    lineStyleComboBox->addItem("点线", QVariant::fromValue(Qt::DotLine));


    connect(lineStyleComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::setLineStyle);

    // 创建橡皮擦按钮
    eraserButton = new QPushButton("🧽 橡皮擦", this);
    connect(eraserButton, &QPushButton::clicked, this, &MainWindow::selectEraser);

    // 添加到工具栏
    toolBar->addWidget(colorButton);
    toolBar->addWidget(clearButton);
    toolBar->addWidget(penWidthButton);
    toolBar->addWidget(modeComboBox);
    toolBar->addWidget(lineStyleComboBox);
    toolBar->addWidget(eraserButton);
}

void MainWindow::applyStyleSheet() {
    this->setStyleSheet(R"(
        QMainWindow {
            background-color: #2E3440;
        }

        QToolBar {
            background-color: #E0E0E0;
            padding: 5px;
            border-bottom: 2px solid #A0A0A0;
        }

        QPushButton {
            background-color: #A0A0A0;
            color: white;
            font-size: 16px;
            border-radius: 8px;
            padding: 8px 16px;
        }

        QPushButton:hover {
            background-color: #81A1C1;
        }

        QComboBox {
            background-color: #A0A0A0;
            color: white;
            font-size: 16px;
            padding: 8px 16px;
            border-radius: 8px;
        }

        QComboBox:hover {
            background-color: #81A1C1;
        }
    )");
}

void MainWindow::selectColor() {
    QColor color = QColorDialog::getColor(Qt::black, this, "选择颜色");
    if (color.isValid()) {
        canvas->setPenColor(color);
    }
}

void MainWindow::clearCanvas() {
    canvas->clearCanvas();
}

void MainWindow::setPenWidth() {
    bool ok;
    int width = QInputDialog::getInt(this, "设置画笔粗细", "粗细:", 2, 1, 10, 1, &ok);
    if (ok) {
        canvas->setPenWidth(width);
    }
}
void MainWindow::setLineStyle(int index) {
    if (index < 0 || index >= lineStyleComboBox->count()) return;  // 避免越界访问

    Qt::PenStyle style = lineStyleComboBox->itemData(index).value<Qt::PenStyle>();
    if (canvas) {
        canvas->setLineStyle(style);
    }
}



void MainWindow::setDrawingMode(int index) {
    canvas->setDrawingMode(index);
}

void MainWindow::selectEraser() {
    bool ok;
    int width = QInputDialog::getInt(this, "设置橡皮擦粗细", "粗细:", 6, 1, 20, 1, &ok);
    if (ok) {
        canvas->setDrawingMode(3);  // 设置为橡皮擦模式
        canvas->setPenWidth(width);
    }
}

