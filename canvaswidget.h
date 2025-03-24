#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

class CanvasWidget : public QWidget {
    Q_OBJECT

public:
    explicit CanvasWidget(QWidget *parent = nullptr);
    void setPenColor(QColor color);
    void setPenWidth(int width);
    void clearCanvas();
    void setDrawingMode(int mode);
    void setLineStyle(Qt::PenStyle style);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QImage canvasImage;
    QColor penColor;
    int penWidth;
    bool drawing;
    QPoint startPoint, endPoint;
    int drawingMode;  // 0: 自由绘制, 1: 直线, 2: 圆弧
    Qt::PenStyle lineStyle = Qt::SolidLine;

    void drawBresenhamLine(QPainter &painter, QPoint p1, QPoint p2);
    void drawMidpointArc(QPainter &painter, QPoint center, int radius, int startAngle, int endAngle);
};

#endif // CANVASWIDGET_H
