#ifndef CANVASWIDGET_H
#define CANVASWIDGET_H

#include <QWidget>
#include <QPainter>
#include <QMouseEvent>

class CanvasWidget : public QWidget {
    Q_OBJECT

public:
    enum Connectivity { FourWay, EightWay };  // 枚举必须首先声明
    enum ClipAlgorithm { CohenSutherland, MidpointSubdivision };
    enum LineAlgorithm { Bresenham, Midpoint };
    explicit CanvasWidget(QWidget *parent = nullptr);
    void setPenColor(QColor color);
    void setPenWidth(int width);
    void clearCanvas();
    void setDrawingMode(int mode);
    void setLineStyle(Qt::PenStyle style);
    void setFillConnectivity(Connectivity conn);
    void setClipAlgorithm(ClipAlgorithm algo);
    void setLineAlgorithm(LineAlgorithm algo);
    double zoomFactor() const { return m_zoomFactor; }
    void setZoom(double factor);
    void resetZoom();
    QPoint mapToCanvas(const QPoint& pos) const;
    QPoint mapFromCanvas(const QPoint& canvasPos) const;
    QVector<QLine> getDrawnLines();  // 添加获取已绘制线段的函数声明

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
    QRect clipWindow;
    QVector<QLine> clippedLines;
    ClipAlgorithm clipAlgorithm = CohenSutherland;
    QRect clipRect; // 裁剪框
    bool isDraggingClipRect = false; // 是否正在拖动裁剪框
    QPoint clipStartPoint; // 裁剪框的起始点
    LineAlgorithm lineAlgorithm = Bresenham; // 默认直线算法
    void floodFill(QPoint seedPoint);  // 函数声明

    QPointF mapToImage(const QPoint& pos) const;
    QPointF mapFromImage(const QPointF& imagePos) const;
    void drawBresenhamLine(QPainter &painter, QPoint p1, QPoint p2);
    void drawMidpointLine(QPainter &painter, QPoint p1, QPoint p2); // 添加中点算法声明
    void drawMidpointArc(QPainter &painter, QPoint center, int radius, int startAngle, int endAngle);
    int computeOutCode(const QPoint &p) const;
    bool cohenSutherlandClip(QLine &line);
    void midpointSubdivisionClip(QLine line);
    void processClipping();
};

#endif // CANVASWIDGET_H
