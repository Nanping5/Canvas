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
    
    // 绘制实时预览
    if (drawing && drawingMode != 0) {
        QColor previewColor = penColor;
        previewColor.setAlpha(128);  // 50%透明度
        painter.setPen(QPen(previewColor, penWidth, lineStyle));
        painter.setRenderHint(QPainter::Antialiasing);

        switch (drawingMode) {
        case 1: // 直线预览
            painter.drawLine(startPoint, currentPoint);
            break;
        case 2: { // 圆预览
            int radius = static_cast<int>(sqrt(pow(currentPoint.x() - startPoint.x(), 2) +
                                             pow(currentPoint.y() - startPoint.y(), 2)));
            painter.drawEllipse(startPoint, radius, radius);
            break;
        }
        }
    }
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
    if (drawing) {
        currentPoint = event->pos();  // 始终更新当前点坐标
        
        if (drawingMode == 0) {
            QPainter painter(&canvasImage);
            painter.setRenderHint(QPainter::Antialiasing, true);
            QPen pen(penColor, penWidth, lineStyle, Qt::RoundCap, Qt::RoundJoin);
            if(lineStyle != Qt::SolidLine) {
                QList<qreal> dashPattern = {5, 5};
                if(lineStyle == Qt::DotLine) dashPattern[0] = 1, dashPattern[1] = 5;
                pen.setDashPattern(dashPattern);
            }
            painter.setPen(pen);

            // 提高虚线的可见性
            QPainterPath path;
            path.moveTo(startPoint);
            path.lineTo(event->pos());
            painter.drawPath(path);

            startPoint = event->pos();
            update();
        } else {
            update();
        }
    }
}





void CanvasWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (drawing) {
        drawing = false;
        endPoint = event->pos();
        currentPoint = endPoint;  // 同步最终坐标
        
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

    QPen solidPen(penColor, penWidth);  // 使用实线绘制单个点
    painter.setPen(solidPen);

    int dashCounter = 0;
    bool drawPixel = true;
    int dashLength = (lineStyle == Qt::DotLine) ? 2 : 5;  // 点线间隔更短

    while (true) {
        if(lineStyle != Qt::SolidLine) {
            if(dashCounter % (dashLength*2) < dashLength) {
                drawPixel = true;
                if(lineStyle == Qt::DotLine) drawPixel = (dashCounter % 4 < 2);  // 点线模式
            } else {
                drawPixel = false;
            }
        }
        
        if(drawPixel) {
            painter.drawPoint(x1, y1);
        }
        dashCounter++;

        if (x1 == x2 && y1 == y2) break;
        int e2 = err * 2;
        if (e2 > -dy) { err -= dy; x1 += sx; }
        if (e2 < dx) { err += dx; y1 += sy; }
    }
}

void CanvasWidget::drawMidpointArc(QPainter &painter, QPoint center, int radius, int, int) {
    QPen solidPen(penColor, penWidth);
    painter.setPen(solidPen);
    int dashCounter = 0;
    const int dashLength = (lineStyle == Qt::DotLine) ? 2 : 5;

    int x = radius;
    int y = 0;
    int p = 1 - radius;

    while (x >= y) {
        bool drawPixel = true;
        if(lineStyle != Qt::SolidLine) {
            if(dashCounter % (dashLength*2) >= dashLength) {
                drawPixel = false;
            }
            if(lineStyle == Qt::DotLine && dashCounter % 4 >= 2) {
                drawPixel = false;
            }
        }
        dashCounter++;
        
        if(drawPixel) {
            painter.drawPoint(center.x() + x, center.y() - y);
            painter.drawPoint(center.x() + y, center.y() - x);
            painter.drawPoint(center.x() - y, center.y() - x);
            painter.drawPoint(center.x() - x, center.y() - y);
            painter.drawPoint(center.x() - x, center.y() + y);
            painter.drawPoint(center.x() - y, center.y() + x);
            painter.drawPoint(center.x() + y, center.y() + x);
            painter.drawPoint(center.x() + x, center.y() + y);
        }

        y++;
        if (p <= 0) {
            p += 2 * y + 1;
        } else {
            x--;
            p += 2 * (y - x) + 1;
        }
    }
}
