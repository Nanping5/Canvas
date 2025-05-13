#ifndef ANIMATIONWINDOW_H
#define ANIMATIONWINDOW_H

#include <QWidget>
#include <QPushButton>
#include <QTimer>
#include <QList>
#include <QLabel>
#include <QResizeEvent>
#include "particle.h"
#include "canvaswidget.h"

enum ParticleEffect {
    LineBresenham,
    LineMidpoint,
    Circle
};

class AnimationWindow : public QWidget {
    Q_OBJECT
public:
    enum ColorMode {
        SingleColorPerFirework,
        RandomColorPerParticle
    };
    Q_ENUM(ColorMode)

    explicit AnimationWindow(QWidget *parent = nullptr);
    ~AnimationWindow();
    
    void setLineAlgorithm(CanvasWidget::LineAlgorithm algo);
    void updateAlgorithmDisplay();
    CanvasWidget::LineAlgorithm currentAlgorithm() const;
    void setParticleEffect(ParticleEffect effect);
    void setColorMode(ColorMode mode);
    ParticleEffect currentEffect() const { return m_effect; }
    void clearCanvas();
    
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateParticles();
    void launchFirework();
    void launchFirework(const QPointF &pos);

private:
    QPushButton *backButton;
    QPushButton *algoButton;
    QLabel *algorithmLabel;
    QTimer *m_timer;
    QList<Particle> m_particles;
    QList<QList<Particle>> m_fireworks;
    QColor randomColor() const;
    CanvasWidget *canvas;
    ParticleEffect m_effect = LineBresenham;
    ColorMode m_colorMode = SingleColorPerFirework;
    
signals:
    void backToMain();
};

#endif // ANIMATIONWINDOW_H 