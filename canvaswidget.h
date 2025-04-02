#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

class CanvasWidget : public QWidget {
    Q_OBJECT

public:
    enum Connectivity { FourWay, EightWay };
    explicit CanvasWidget(QWidget *parent = nullptr);
    void setPenColor(QColor color);
    void setPenWidth(int width);
    void clearCanvas();
    void setDrawingMode(int mode);
    void setLineStyle(Qt::PenStyle style);
    void setFillConnectivity(Connectivity conn);
    double zoomFactor() const { return m_zoomFactor; }
    void setZoom(double factor);
    void resetZoom();
    QPoint mapToCanvas(const QPoint& pos) const;
    QPoint mapFromCanvas(const QPoint& canvasPos) const;

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
    int drawingMode;  // 0:自由绘制,1:直线,2:圆,3:橡皮擦,4:多边形,5:填充
    Qt::PenStyle lineStyle = Qt::SolidLine;
    QColor backgroundColor;
    double m_zoomFactor = 1.0;
    QPointF m_zoomOffset;
    QPoint m_lastDragPos;
    QPoint m_canvasOffset;
    QVector<QPoint> polygonPoints;
    bool isFirstEdge;
    QPoint firstVertex;
    const int CLOSE_DISTANCE = 20;
    Connectivity fillConnectivity = EightWay;
    void floodFill(QPoint seedPoint);  // 函数声明

    QPointF mapToImage(const QPoint& pos) const;
    QPointF mapFromImage(const QPointF& imagePos) const;
    void drawBresenhamLine(QPainter &painter, QPoint p1, QPoint p2);
    void drawMidpointArc(QPainter &painter, QPoint center, int radius, int startAngle, int endAngle);
};

#endif // CANVASWIDGET_H
