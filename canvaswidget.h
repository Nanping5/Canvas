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
    double zoomFactor() const { return m_zoomFactor; }
    void setZoom(double factor);
    void resetZoom();

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QImage canvasImage;
    QColor penColor;
    int penWidth;
    bool drawing;
    QPoint startPoint, endPoint, currentPoint;
    int drawingMode;  // 0: 自由绘制, 1: 直线, 2: 圆弧
    Qt::PenStyle lineStyle = Qt::SolidLine;
    QColor backgroundColor;  // 用于存储画布的背景色
    double m_zoomFactor = 1.0;     // 原zoomFactor
    QPointF m_zoomOffset;          // 原zoomOffset
    QPoint m_lastDragPos;          // 原lastDragPos
    QPoint m_canvasOffset; // 新增画布偏移量

    // 坐标转换方法
    QPointF mapToImage(const QPoint& pos) const;
    QPointF mapFromImage(const QPointF& imagePos) const;  // 新增

    void drawBresenhamLine(QPainter &painter, QPoint p1, QPoint p2);
    void drawMidpointArc(QPainter &painter, QPoint center, int radius, int startAngle, int endAngle);
};

#endif // CANVASWIDGET_H
