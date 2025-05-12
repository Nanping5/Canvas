//@author Nanping5
//@date 2025/3/24
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
    enum TransformMode { None, Rotate, Scale }; // 变换模式
    /**
     * 在Bezier曲线模式下，右键点击可以完成曲线绘制并将其保存到画布上
     */
    explicit CanvasWidget(QWidget *parent = nullptr);
    void setPenColor(QColor color);
    void setPenWidth(int width);
    void clearCanvas();
    void setDrawingMode(int mode);
    void setLineStyle(Qt::PenStyle style);
    void setFillConnectivity(Connectivity conn);
    void setClipAlgorithm(ClipAlgorithm algo);
    void setLineAlgorithm(LineAlgorithm algo);
    void setSelectionMode(bool enabled);
    double zoomFactor() const { return m_zoomFactor; }
    void setZoom(double factor);
    void resetZoom();
    QPoint mapToCanvas(const QPoint& pos) const;
    QPoint mapFromCanvas(const QPoint& canvasPos) const;
    QVector<QLine> getDrawnLines();  // 添加获取已绘制线段的函数声明
    int selectionMode = 0; // 0: normal, 1: select, 2: move
    bool saveImage(const QString &fileName, const char *format = nullptr);
    void setTransformMode(TransformMode mode);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;  // 添加键盘事件处理

private:
    QImage canvasImage;
    QImage originalCanvas;  // 用于存储原始画布状态
    QColor penColor;
    int penWidth;
    bool drawing;
    QPoint startPoint, endPoint, currentPoint;
    int drawingMode;  // 0:自由绘制,1:直线,2:圆,3:橡皮擦,4:多边形,5:填充,6:裁剪,7:选择
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
    QRect selectionRect; // 选择框
    bool isDraggingSelection = false; // 是否正在拖动选择框
    QPoint selectionStartPoint; // 选择框的起始点
    QImage selectionImage; // 存储选择的图像
    bool isSelecting = false; // 是否正在选择
    bool isMoving = false; // 是否正在移动
    QPoint selectionOffset; // 移动时的偏移量
    QVector<QPoint> controlPoints; // 存储控制点
    QVector<QVector<QPoint>> allPolygons;     // 存储所有已绘多边形
    QVector<QVector<QPoint>> clippedPolygons; // 存储裁剪后的多边形
    QVector<QLine> originalLines;             // 存储原始线段
    void floodFill(QPoint seedPoint);  // 函数声明
    bool inside(const QPoint& p, int edge, int xmin, int ymin, int xmax, int ymax);
    QPoint computeIntersection(QPoint p1, QPoint p2, int edge, 
                              int xmin, int ymin, int xmax, int ymax);

    QPointF mapToImage(const QPoint& pos) const;
    QPointF mapFromImage(const QPointF& imagePos) const;
    void drawBresenhamLine(QPainter &painter, QPoint p1, QPoint p2);
    void drawMidpointLine(QPainter &painter, QPoint p1, QPoint p2); // 添加中点算法声明
    void drawMidpointArc(QPainter &painter, QPoint center, int radius, 
                        double startAngle, double endAngle, bool isFullCircle = false);
    int computeOutCode(const QPoint &p) const;
    bool cohenSutherlandClip(QLine &line);
    void midpointSubdivisionClip(QLine line);
    void processClipping();
    void drawBezierCurve(QPainter &painter);
    QPoint deCasteljau(const QVector<QPoint> &points, double t);
    void drawArcPoint(QPainter &painter, QPoint center, int x, int y, int dashCounter, int dashLength);
    void clipPolygons(); // 多边形裁剪函数

    TransformMode transformMode = None;
    QPoint rotateCenter;
    double rotateAngle = 0;
    bool isRotating = false;
    QImage preTransformImage; // 变换前的图像副本
    double initialAngle = 0;
    double currentAngle = 0;

    QRect scaleRect; // 缩放选区
    QImage scaleOriginal; // 原始选区图像
    double scaleFactor = 1.0; // 当前缩放比例
    bool isScaling = false; // 是否正在缩放

    bool isAdjustingCurve = false; // 是否正在调整曲线
    int selectedPointIndex = -1;   // 当前选中的控制点索引
    QImage curvePreviewImage;      // 曲线预览临时图像

signals:
    void imageModified();
    void clippingConfirmed();

public slots:
    void confirmClipping();
};

#endif // CANVASWIDGET_H
