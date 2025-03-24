#include "canvaswidget.h"
#include <QPainterPath>
#include<cmath>
CanvasWidget::CanvasWidget(QWidget *parent) : QWidget(parent), penColor(Qt::black), penWidth(2), drawing(false), drawingMode(0) {
    canvasImage = QImage(1440, 1024, QImage::Format_ARGB32);
    canvasImage.fill(Qt::white);
    lineStyle = Qt::SolidLine;  // 默认使用实线

}

void CanvasWidget::setPenColor(QColor color) {
    penColor = color;
}

void CanvasWidget::setPenWidth(int width) {
    penWidth = width;
}

void CanvasWidget::clearCanvas() {
    canvasImage.fill(Qt::white);
    update();
}

void CanvasWidget::setDrawingMode(int mode) {
    drawingMode = mode;
}
void CanvasWidget::setLineStyle(Qt::PenStyle style) {
    lineStyle = style;
}



void CanvasWidget::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.drawImage(0, 0, canvasImage);
}

void CanvasWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        drawing = true;
        startPoint = event->pos();
        endPoint = startPoint;
    }
}


void CanvasWidget::resizeEvent(QResizeEvent *event) {
    if (width() > canvasImage.width() || height() > canvasImage.height()) {
        // 创建一个新的 QImage 并填充背景色
        QImage newImage(width(), height(), QImage::Format_ARGB32);
        newImage.fill(Qt::white);

        // 把原来的画布内容拷贝到新的 QImage 上
        QPainter painter(&newImage);
        painter.drawImage(0, 0, canvasImage);

        // 更新画布
        canvasImage = newImage;
    }
    QWidget::resizeEvent(event);
}

void CanvasWidget::mouseMoveEvent(QMouseEvent *event) {
    if (drawing && drawingMode == 0) {
        QPainter painter(&canvasImage);
        painter.setRenderHint(QPainter::Antialiasing, true);  // 抗锯齿
        QPen pen(penColor, penWidth, lineStyle, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);

        // 使用 QPainterPath 来提高虚线的可见性
        QPainterPath path;
        path.moveTo(startPoint);
        path.lineTo(event->pos());
        painter.drawPath(path);

        startPoint = event->pos();
        update();
    }
}





void CanvasWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (drawing) {
        drawing = false;
        endPoint = event->pos();
        QPainter painter(&canvasImage);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QPen pen(penColor, penWidth, lineStyle);
        painter.setPen(pen);

        switch (drawingMode) {
        case 1:
            drawBresenhamLine(painter, startPoint, endPoint);
            break;
        case 2:
            int radius = static_cast<int>(sqrt(pow(endPoint.x() - startPoint.x(), 2) +
                                               pow(endPoint.y() - startPoint.y(), 2)));
            drawMidpointArc(painter, startPoint, radius, 0, 360);
            break;
        }
        update();
    }
}

void CanvasWidget::drawBresenhamLine(QPainter &painter, QPoint p1, QPoint p2) {
    int x1 = p1.x(), y1 = p1.y();
    int x2 = p2.x(), y2 = p2.y();
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    QPen pen(penColor, penWidth, lineStyle);
    painter.setPen(pen);

    while (true) {
        painter.drawPoint(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

void CanvasWidget::drawMidpointArc(QPainter &painter, QPoint center, int radius, int, int) {
    int x = radius;
    int y = 0;
    int p = 1 - radius;

    while (x >= y) {
        // 绘制八个对称点
        painter.drawPoint(center.x() + x, center.y() - y);
        painter.drawPoint(center.x() + y, center.y() - x);
        painter.drawPoint(center.x() - y, center.y() - x);
        painter.drawPoint(center.x() - x, center.y() - y);
        painter.drawPoint(center.x() - x, center.y() + y);
        painter.drawPoint(center.x() - y, center.y() + x);
        painter.drawPoint(center.x() + y, center.y() + x);
        painter.drawPoint(center.x() + x, center.y() + y);

        y++;
        if (p <= 0) {
            p += 2 * y + 1;
        } else {
            x--;
            p += 2 * (y - x) + 1;
        }
    }
}
