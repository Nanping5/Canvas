#include "animationwindow.h"
#include <QVBoxLayout>  // 添加垂直布局头文件
#include <QPainter>
#include <QRandomGenerator>
#include <QtMath>
#include <QDebug>

const int PARTICLE_COUNT = 100;
const int FIREWORK_LIFESPAN = 100;

AnimationWindow::AnimationWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("动画窗口");
    setMinimumSize(1440, 960);  // 设置最小大小，但允许用户调整窗口大小
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMouseTracking(true);  // 启用鼠标跟踪
    
    canvas = new CanvasWidget(this);
    canvas->setBackgroundColor(Qt::black); // 设置动画窗口画布背景
    canvas->setDrawingMode(CanvasWidget::Bresenham);  // 设置默认算法
    canvas->setLineStyle(Qt::DotLine);  // 使用点线样式
    canvas->setPenWidth(1);             // 固定线宽
    canvas->setMouseTransparent(true);  // 启用鼠标穿透
    canvas->setAttribute(Qt::WA_TransparentForMouseEvents);  // 确保canvas不拦截鼠标事件

    backButton = new QPushButton("返回主界面", this);
    algorithmLabel = new QLabel("当前算法: Bresenham", this);
    algorithmLabel->setStyleSheet(
        "color: rgba(255, 255, 255, 200);"
        "font-size: 16px;"
        "background: rgba(0, 0, 0, 100);"
        "padding: 5px;"
        "border-radius: 5px;"  // 添加圆角
    );
    algorithmLabel->setAlignment(Qt::AlignRight | Qt::AlignBottom);
    algorithmLabel->adjustSize();  // 自动调整大小

    // 创建切换算法按钮
    algoButton = new QPushButton("切换算法", this);
    connect(algoButton, &QPushButton::clicked, [this](){
        ParticleEffect effects[] = {LineBresenham, LineMidpoint, Circle};
        int next = (static_cast<int>(m_effect) + 1) % 3;
        setParticleEffect(effects[next]);
    });

    QPushButton *colorModeButton = new QPushButton("颜色模式", this);
    connect(colorModeButton, &QPushButton::clicked, [this](){
        ColorMode modes[] = {SingleColorPerFirework, RandomColorPerParticle};
        int next = (static_cast<int>(m_colorMode) + 1) % 2;
        setColorMode(static_cast<ColorMode>(next));
    });

    // 布局调整
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(backButton);
    buttonLayout->addWidget(algoButton);
    buttonLayout->addWidget(colorModeButton);
    buttonLayout->addStretch();

   

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(buttonLayout);
    layout->addWidget(canvas);
    layout->setContentsMargins(0, 0, 0, 0);  // 设置边距为0
    layout->setSpacing(0);                   // 设置间距为0
    canvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);  // 设置画布扩展策略

    setLayout(layout);
    
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, &AnimationWindow::updateParticles);
    m_timer->start(16); // ~60 FPS

    // 初始烟花
    QTimer::singleShot(3000, this, static_cast<void (AnimationWindow::*)()>(&AnimationWindow::launchFirework));  // 3秒
    QTimer::singleShot(6000, this, static_cast<void (AnimationWindow::*)()>(&AnimationWindow::launchFirework));  // 6秒
    QTimer::singleShot(9000, this, static_cast<void (AnimationWindow::*)()>(&AnimationWindow::launchFirework));  // 9秒

    connect(backButton, &QPushButton::clicked, [this](){
        emit backToMain();
        close();
    });
}

AnimationWindow::~AnimationWindow() {
    m_timer->stop();
}

void AnimationWindow::paintEvent(QPaintEvent *event) {
    QPainter painter(this);  // 必须创建QPainter实例
    
    // 清空并重置画布背景
    canvas->clearCanvas();
    
    // 绘制所有粒子轨迹（永久保留）
    for (const auto& firework : m_fireworks) {
        for (const auto& p : firework) {
            if(m_effect == Circle) {
                canvas->drawCircle(p.position().toPoint(), 
                                 p.size()/2, 
                                 p.color(),
                                 qMax(1, p.size()/4));
            } else {
                canvas->drawLine(
                    p.position().toPoint(),
                    p.position().toPoint() + p.velocity().toPoint() * 3,
                    p.color(),
                    qMax(2, p.size()/2)
                );
            }
        }
    }
    
    canvas->render(&painter);  // 正确使用QPainter指针
}

void AnimationWindow::updateParticles() {
    // 更新现有粒子
    for (auto& firework : m_fireworks) {
        for (auto& p : firework) {
            p.update();
        }
    }

    // 移除过期粒子
    m_fireworks.erase(std::remove_if(m_fireworks.begin(), m_fireworks.end(),
        [](const QList<Particle>& fw) {
            return fw.empty() || fw.first().isDead();
        }), m_fireworks.end());

    update();
}

void AnimationWindow::launchFirework() {
    QList<Particle> firework;
    QPointF startPos(width()/2, height());
    QColor baseColor = randomColor();

    for (int i = 0; i < PARTICLE_COUNT; ++i) {
        float angle = 2 * M_PI * i / PARTICLE_COUNT;
        float speed = QRandomGenerator::global()->generateDouble() * 4.0 + 8.0;  // 生成8.0-12.0之间的浮点数
        QPointF vel(speed * qCos(angle), speed * qSin(angle));
        firework.append(Particle(startPos, vel, baseColor, FIREWORK_LIFESPAN));
    }
    
    m_fireworks.append(firework);
    
    // 随机间隔发射下一个烟花
    QTimer::singleShot(QRandomGenerator::global()->bounded(3000, 6000),  // 3-6秒随机间隔
                      this, static_cast<void (AnimationWindow::*)()>(&AnimationWindow::launchFirework));
}

void AnimationWindow::launchFirework(const QPointF &pos) {
    QList<Particle> firework;
    QColor baseColor = (m_colorMode == SingleColorPerFirework) ? 
                      randomColor() : Qt::transparent;

    // 随机生成粒子参数
    int particleCount = QRandomGenerator::global()->bounded(50, 200);
    int maxSpeed = QRandomGenerator::global()->bounded(8, 12);
    int lifespan = 0;
    switch(m_effect) {
    case Circle:
        lifespan = 150;
        break;
    default:
        lifespan = QRandomGenerator::global()->bounded(80, 120);
    }

    for (int i = 0; i < particleCount; ++i) {
        // 计算粒子方向
        float angle = 2 * M_PI * i / particleCount;
        float speed = maxSpeed * 0.8f + QRandomGenerator::global()->generateDouble() * maxSpeed * 0.4f;
        QPointF vel(speed * qCos(angle), speed * qSin(angle));
        
        // 生成粒子属性
        int size = QRandomGenerator::global()->bounded(3, 9);
        QColor particleColor = randomColor().darker(QRandomGenerator::global()->bounded(100, 200));
        
        if (m_colorMode == SingleColorPerFirework) {
            particleColor = baseColor.darker(QRandomGenerator::global()->bounded(100, 200));
        }
        
        // 使用CanvasWidget的算法绘制轨迹
        QPoint start = pos.toPoint();
        QPoint end = start + vel.toPoint() * 10;
        
        // 绘制粒子轨迹
        canvas->drawLine(start, end, particleColor, size);

        firework.append(Particle(pos, vel, particleColor, lifespan, size));
    }
    
    m_fireworks.append(firework);
}

void AnimationWindow::setLineAlgorithm(CanvasWidget::LineAlgorithm algo) {
    if(canvas) {
        canvas->setLineAlgorithm(algo);
    }
}

void AnimationWindow::setParticleEffect(ParticleEffect effect) {
    m_effect = effect;
    QString effectName;
    switch(effect) {
    case LineBresenham:
        canvas->setLineAlgorithm(CanvasWidget::Bresenham);
        effectName = "Bresenham线条";
        break;
    case LineMidpoint:
        canvas->setLineAlgorithm(CanvasWidget::Midpoint);
        effectName = "中点线条";
        break;
    case Circle:
        effectName = "圆形粒子";
        break;
    }
    algorithmLabel->setText("当前效果: " + effectName);
}

QColor AnimationWindow::randomColor() const {
    return QColor::fromHsv(QRandomGenerator::global()->bounded(360), 
                          255, QRandomGenerator::global()->bounded(200, 255),  // 随机饱和度
                          QRandomGenerator::global()->bounded(200, 255)); // 随机亮度
}

void AnimationWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        qDebug() << "Mouse clicked at:" << event->pos() 
                 << "Window geometry:" << geometry()
                 << "Canvas geometry:" << canvas->geometry();
        
        // 确保点击在画布区域内
        if (canvas->geometry().contains(event->pos())) {
            launchFirework(event->pos());
        }
    }
    QWidget::mousePressEvent(event);
}

void AnimationWindow::updateAlgorithmDisplay() {
    CanvasWidget::LineAlgorithm algo = canvas->getLineAlgorithm();
    QString algoName = (algo == CanvasWidget::Bresenham) ? "Bresenham" : "中点算法";
    algorithmLabel->setText("当前算法: " + algoName);
}

void AnimationWindow::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    // 保持标签在右下角
    algorithmLabel->move(width() - algorithmLabel->width() - 20, 
                       height() - algorithmLabel->height() - 20);
}

void AnimationWindow::clearCanvas() {
    // 仅清除画布区域
    QPainter painter(this);
    painter.fillRect(canvas->geometry(), Qt::black); // 仅填充画布区域
    canvas->clearCanvas();
    update();
}

void AnimationWindow::setColorMode(ColorMode mode) {
    m_colorMode = mode;
    algorithmLabel->setText(
        QString("颜色模式: %1").arg(mode == SingleColorPerFirework ? "单色烟花" : "多彩粒子")
    );
} 
