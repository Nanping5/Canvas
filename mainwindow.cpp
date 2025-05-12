#include "mainwindow.h"
#include <QColorDialog>
#include <QInputDialog>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>

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
    colorButton = new QPushButton("颜色", this);
    connect(colorButton, &QPushButton::clicked, this, &MainWindow::selectColor);

    // 清除按钮
    clearButton = new QPushButton("清除", this);
    connect(clearButton, &QPushButton::clicked, this, &MainWindow::clearCanvas);

    // 画笔粗细按钮
    penWidthButton = new QPushButton("粗细", this);
    connect(penWidthButton, &QPushButton::clicked, this, &MainWindow::setPenWidth);

    // 选择绘制模式
    modeComboBox = new QComboBox(this);
    modeComboBox->addItem("自由绘制");
    modeComboBox->addItem("直线-Bresenham");
    modeComboBox->addItem("直线-中点");
    modeComboBox->addItem("圆");
    modeComboBox->addItem("多边形");
    modeComboBox->addItem("Bezier曲线");
    modeComboBox->addItem("圆弧-中点");
    connect(modeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::setDrawingMode);

    // 线型选择框
    lineStyleComboBox = new QComboBox(this);
    lineStyleComboBox->addItem("实线", QVariant::fromValue(Qt::SolidLine));
    lineStyleComboBox->addItem("虚线", QVariant::fromValue(Qt::DashLine));
    lineStyleComboBox->addItem("点线", QVariant::fromValue(Qt::DotLine));


    connect(lineStyleComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::setLineStyle);

    // 创建橡皮擦按钮
    eraserButton = new QPushButton("橡皮擦", this);
    connect(eraserButton, &QPushButton::clicked, this, &MainWindow::selectEraser);

    // 添加填充按钮
    QPushButton *fillButton = new QPushButton("填充", this);
    fillButton->setCheckable(true);
    fillButton->setAutoExclusive(true);
    connect(fillButton, &QPushButton::toggled, this, [this](bool checked) {
        canvas->setDrawingMode(5 ? 5 : -1);  // 切换模式
    });

    // 添加裁剪模式和算法选择
    QComboBox *clipCombo = new QComboBox(this);
    clipCombo->addItem("裁剪模式 - Cohen-Sutherland", QVariant::fromValue(CanvasWidget::CohenSutherland));
    clipCombo->addItem("裁剪模式 - 中点分割", QVariant::fromValue(CanvasWidget::MidpointSubdivision));

    // 连接裁剪模式选择的信号
    connect(clipCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [=](int index) {
        QVariant data = clipCombo->itemData(index);
        if (data.isValid()) {
            CanvasWidget::ClipAlgorithm algo = data.value<CanvasWidget::ClipAlgorithm>();
            canvas->setClipAlgorithm(algo);
            canvas->setDrawingMode(6); // 6 是裁剪模式的标识
        }
    });

    // 添加选择按钮
    QPushButton *selectButton = new QPushButton("选中", this);
    selectButton->setCheckable(true);
    connect(selectButton, &QPushButton::toggled, this, [this](bool checked) {
        canvas->setSelectionMode(checked);
    });

    // 添加保存按钮
    QPushButton *saveButton = new QPushButton("💾 保存", this);
    connect(saveButton, &QPushButton::clicked, this, &MainWindow::saveCanvas);

    // 添加旋转按钮
    QPushButton *rotateButton = new QPushButton("旋转", this);
    connect(rotateButton, &QPushButton::clicked, this, [this]() {
        canvas->setTransformMode(CanvasWidget::Rotate);
    });

    // 添加缩放按钮
    QPushButton *scaleButton = new QPushButton("缩放", this);
    connect(scaleButton, &QPushButton::clicked, this, [this]() {
        canvas->setTransformMode(CanvasWidget::Scale);
    });

    // 添加到工具栏
    toolBar->addWidget(saveButton);
    toolBar->addWidget(colorButton);
    toolBar->addWidget(clearButton);
    toolBar->addWidget(penWidthButton);
    toolBar->addWidget(modeComboBox);
    toolBar->addWidget(lineStyleComboBox);
    toolBar->addWidget(eraserButton);
    toolBar->addWidget(fillButton);
    toolBar->addWidget(clipCombo);
    toolBar->addWidget(selectButton);
    toolBar->addWidget(rotateButton);
    toolBar->addWidget(scaleButton);
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

        QPushButton[text="🪣 填充"] {
            background-color: #88C0D0;
            border: 2px solid #5E81AC;
            color: #2E3440;
        }
        QPushButton[text="🪣 填充"]:hover {
            background-color: #81A1C1;
            border-color: #4C6793;
        }
        QPushButton[text="🪣 填充"]:checked {
            background-color: #5E81AC;
            border-color: #4C6793;
            color: white;
        }
        QPushButton[text="🪣 填充"]:disabled {
            background-color: #D8DEE9;
            color: #999999;
        }

        QPushButton[text="🔍 选择"] {
            background-color: #A3BE8C;
            border: 2px solid #8FBCBB;
            color: #2E3440;
        }
        QPushButton[text="🔍 选择"]:hover {
            background-color: #8FBCBB;
            border-color: #88C0D0;
        }
        QPushButton[text="🔍 选择"]:checked {
            background-color: #88C0D0;
            border-color: #81A1C1;
            color: white;
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
    int modeMap[] = {0, 1, 1, 2, 4, 7, 8};
    if (index >= 0 && index < 7) {
        canvas->setDrawingMode(modeMap[index]);
        if (index == 1) {
            canvas->setLineAlgorithm(CanvasWidget::Bresenham);
        } else if (index == 2) {
            canvas->setLineAlgorithm(CanvasWidget::Midpoint);
        }
    }
}

void MainWindow::selectEraser() {
    bool ok;
    int width = QInputDialog::getInt(this, "设置橡皮擦粗细", "粗细:", 6, 1, 20, 1, &ok);
    if (ok) {
        canvas->setDrawingMode(3);  // 设置为橡皮擦模式
        canvas->setPenWidth(width);
    }
}

void MainWindow::saveCanvas() {
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("保存图像"), "",
        tr("PNG 文件 (*.png);;JPEG 文件 (*.jpg *.jpeg);;BMP 文件 (*.bmp);;所有文件 (*)"));
    
    if (!fileName.isEmpty()) {
        if (!canvas->saveImage(fileName)) {
            QMessageBox::warning(this, tr("保存失败"), tr("无法保存图像文件。"));
        }
    }
}
