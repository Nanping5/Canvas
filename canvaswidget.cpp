#include "canvaswidget.h"
#include <QPainterPath>
#include<cmath>
#include <QQueue>

CanvasWidget::CanvasWidget(QWidget *parent) : QWidget(parent), penColor(Qt::black), penWidth(2), drawing(false), drawingMode(0) {
    canvasImage = QImage(1440, 1024, QImage::Format_ARGB32);
    canvasImage.fill(Qt::white);
    lineStyle = Qt::SolidLine;  // 默认使用实线
    backgroundColor = Qt::white;  // 默认使用白色作为背景色
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
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    // 应用变换
    painter.translate(m_zoomOffset);
    painter.scale(m_zoomFactor, m_zoomFactor);
    painter.translate(-m_canvasOffset);
    
    // 绘制画布
    painter.drawImage(m_canvasOffset, canvasImage);
    
    // 绘制实时预览
    if (drawing) {
        QColor previewColor = penColor;
        float scaledWidth = penWidth * m_zoomFactor;
        
        if (drawingMode == 3) { // 橡皮擦模式
            previewColor = Qt::gray;
        } else {
            previewColor.setAlpha(128);
        }
        
        painter.setPen(QPen(previewColor, scaledWidth, Qt::SolidLine));
        painter.setRenderHint(QPainter::Antialiasing);

        // 根据不同模式绘制预览
        switch (drawingMode) {
        case 0: // 自由绘制模式
            painter.drawLine(startPoint - m_canvasOffset, currentPoint - m_canvasOffset);
            break;
            
        case 1: // 直线预览
            painter.drawLine(startPoint - m_canvasOffset, currentPoint - m_canvasOffset);
            break;
            
        case 2: { // 圆预览
            QPoint adjustedStart = startPoint - m_canvasOffset;
            QPoint adjustedCurrent = currentPoint - m_canvasOffset;
            int radius = static_cast<int>(sqrt(pow(adjustedCurrent.x() - adjustedStart.x(), 2) +
                                             pow(adjustedCurrent.y() - adjustedStart.y(), 2)));
            painter.drawEllipse(adjustedStart, radius, radius);
            break;
        }
            
        case 3: // 橡皮擦预览
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(QPointF(currentPoint - m_canvasOffset), scaledWidth/2, scaledWidth/2);
            break;
            
        case 4: // 多边形预览
            if (!polygonPoints.isEmpty()) {
                // 绘制已确定的边
                for (int i = 0; i < polygonPoints.size() - 1; ++i) {
                    painter.drawLine(polygonPoints[i] - m_canvasOffset, 
                                   polygonPoints[i + 1] - m_canvasOffset);
                }
                // 绘制当前边
                if (isFirstEdge) {
                    painter.drawLine(polygonPoints.first() - m_canvasOffset, 
                                   currentPoint - m_canvasOffset);
                } else {
                    painter.drawLine(polygonPoints.last() - m_canvasOffset, 
                                   currentPoint - m_canvasOffset);
                }
                // 绘制回到起点的边
                if (polygonPoints.size() > 1 && 
                    QLineF(currentPoint, firstVertex).length() < CLOSE_DISTANCE) {
                    painter.drawLine(currentPoint - m_canvasOffset, 
                                   polygonPoints.first() - m_canvasOffset);
                }
            }
            break;
        }
    }
}

QPointF CanvasWidget::mapToImage(const QPoint& pos) const {
    // 窗口坐标 -> 画布坐标
    return QPointF(pos - m_zoomOffset) / m_zoomFactor + m_canvasOffset;
}

QPointF CanvasWidget::mapFromImage(const QPointF& imagePos) const {
    // 画布坐标 -> 窗口坐标
    return (imagePos - m_canvasOffset) * m_zoomFactor + m_zoomOffset;
}

QPoint CanvasWidget::mapToCanvas(const QPoint& pos) const {
    return mapToImage(pos).toPoint();
}

QPoint CanvasWidget::mapFromCanvas(const QPoint& canvasPos) const {
    return mapFromImage(canvasPos).toPoint();
}

void CanvasWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        m_lastDragPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else if (drawingMode == 5) { // 填充模式
        if (event->button() == Qt::LeftButton) {
            QPointF imagePos = mapToImage(event->pos());
            QPoint point = imagePos.toPoint();
            
            if (canvasImage.rect().contains(point)) {
                floodFill(point);
                update();
            }
        }
    } else if (drawingMode == 4) { // 多边形模式
        if (event->button() == Qt::LeftButton) {
            QPointF imagePos = mapToImage(event->pos());
            QPoint point = imagePos.toPoint();
            
            if (!drawing) { // 开始新多边形
                drawing = true;
                isFirstEdge = true;
                polygonPoints.clear();
                polygonPoints.append(point);
                firstVertex = point;
                startPoint = point;
            } else { // 添加新顶点
                polygonPoints.append(point);
                isFirstEdge = false;
            }
            currentPoint = point;
            update();
        } else if (event->button() == Qt::RightButton && drawing) {
            // 右键完成多边形绘制
            if (polygonPoints.size() >= 3) {
                QPainter painter(&canvasImage);
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setPen(QPen(penColor, penWidth, lineStyle));
                painter.drawPolygon(polygonPoints.data(), polygonPoints.size());
            }
            drawing = false;
            polygonPoints.clear();
            update();
        }
    } else {
        QPointF imagePos = mapToImage(event->pos());
        startPoint = imagePos.toPoint();
        currentPoint = startPoint; // 初始化当前点
        if (event->button() == Qt::LeftButton) {
            drawing = true;
        }
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
    if (event->buttons() & Qt::MiddleButton) {
        QPoint delta = event->pos() - m_lastDragPos;
        m_zoomOffset += delta;
        m_lastDragPos = event->pos();
        update();
        event->accept();
        return;
    } else if (drawing) {  // 处理所有绘制模式的移动
        QPointF imagePos = mapToImage(event->pos());
        currentPoint = imagePos.toPoint();
        
        // 自由绘制模式实时绘制
        if (drawingMode == 0) { 
            QPainter painter(&canvasImage);
            painter.setRenderHint(QPainter::Antialiasing);
            QPen pen(penColor, penWidth, lineStyle, Qt::RoundCap, Qt::RoundJoin);
            if(lineStyle != Qt::SolidLine) {
                QList<qreal> dashPattern = {5, 5};
                if(lineStyle == Qt::DotLine) dashPattern[0] = 1, dashPattern[1] = 5;
                pen.setDashPattern(dashPattern);
            }
            painter.setPen(pen);
            painter.drawLine(startPoint - m_canvasOffset, currentPoint - m_canvasOffset);
            startPoint = currentPoint;
        }
        else if (drawingMode == 3) { // 橡皮擦实时擦除
            QPainter painter(&canvasImage);
            painter.setPen(QPen(backgroundColor, penWidth, Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(startPoint - m_canvasOffset, currentPoint - m_canvasOffset);
            startPoint = currentPoint;
        }
        
        // 多边形模式特殊处理
        if (drawingMode == 4) {
            if (polygonPoints.size() >= 2 && 
                QLineF(currentPoint, firstVertex).length() < CLOSE_DISTANCE) {
                currentPoint = firstVertex;
            }
        }
        
        update();  // 触发重绘更新预览
    }
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else if (drawingMode == 4 && event->button() == Qt::LeftButton) {
        // 检查是否接近第一个顶点
        if (QLineF(currentPoint, firstVertex).length() < CLOSE_DISTANCE && 
            polygonPoints.size() >= 3) {
            // 完成多边形绘制
            QPainter painter(&canvasImage);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(QPen(penColor, penWidth, lineStyle));
            painter.drawPolygon(polygonPoints.data(), polygonPoints.size());
            drawing = false;
            polygonPoints.clear();
            update();
        }
    } else {
        if (drawing) {
            drawing = false;
            // 自由绘制模式不需要额外处理，因为已经实时绘制
            if (drawingMode == 1 || drawingMode == 2 || drawingMode == 3) {
                // 处理其他模式的最终绘制
                QPointF imagePos = mapToImage(event->pos());
                endPoint = imagePos.toPoint();
                
                QPainter painter(&canvasImage);
                painter.setRenderHint(QPainter::Antialiasing, true);

                switch (drawingMode) {
                case 1: // 直线
                    drawBresenhamLine(painter, startPoint - m_canvasOffset, endPoint - m_canvasOffset);
                    break;
                case 2: { // 圆
                    int radius = static_cast<int>(sqrt(pow(endPoint.x() - startPoint.x(), 2) +
                                                       pow(endPoint.y() - startPoint.y(), 2)));
                    drawMidpointArc(painter, startPoint - m_canvasOffset, radius, 0, 360);
                    break;
                }
                case 3: // 橡皮擦
                    painter.setPen(QPen(backgroundColor, penWidth, Qt::SolidLine, Qt::RoundCap));
                    painter.drawLine(startPoint - m_canvasOffset, endPoint - m_canvasOffset);
                    break;
                }
            }
            update();
        }
    }
}

void CanvasWidget::drawBresenhamLine(QPainter &painter, QPoint p1, QPoint p2) {
    // 使用原始画笔宽度
    QPen pen(penColor, penWidth, Qt::SolidLine);
    painter.setPen(pen);
    
    int x1 = p1.x(), y1 = p1.y();
    int x2 = p2.x(), y2 = p2.y();
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

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
    QPen pen(penColor, penWidth, lineStyle);  // 确保使用当前线型
    painter.setPen(pen);
    
    int x = radius;
    int y = 0;
    int p = 1 - radius;
    int dashCounter = 0;
    const int dashLength = (lineStyle == Qt::DotLine) ? 2 : 5;  // 点线间隔更短

    while (x >= y) {
        bool drawPixel = true;
        
        // 处理非实线样式
        if(lineStyle != Qt::SolidLine) {
            // 计算当前点属于虚线周期的哪个阶段
            int cyclePos = dashCounter % (dashLength * 2);
            
            if(lineStyle == Qt::DashLine) {
                drawPixel = (cyclePos < dashLength);  // 前半周期绘制，后半空白
            } 
            else if(lineStyle == Qt::DotLine) {
                drawPixel = (cyclePos < 1);  // 每个周期只绘制第一个点
            }
        }

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
        dashCounter += 2;  // 每个y步进增加两次计数（对称点）
        
        if (p <= 0) {
            p += 2 * y + 1;
        } else {
            x--;
            p += 2 * (y - x) + 1;
        }
    }
}

void CanvasWidget::wheelEvent(QWheelEvent *event) {
    if (event->modifiers() & Qt::ControlModifier) {
        // 获取鼠标位置
        QPointF mousePos = event->position();
        
        // 保存当前画布位置
        QPointF scenePos = mapToImage(mousePos.toPoint());
        
        // 计算新的缩放因子（使用更温和的缩放步长）
        if (event->angleDelta().y() > 0) {
            m_zoomFactor = qMin(5.0, m_zoomFactor * 1.05);
        } else {
            m_zoomFactor = qMax(0.2, m_zoomFactor / 1.05);
        }
        
        // 调整偏移量保持鼠标位置不变
        QPointF newScreenPos = mapFromImage(scenePos);
        m_zoomOffset += (mousePos - newScreenPos);
        
        update();
        event->accept();
    } else {
        QWidget::wheelEvent(event);
    }
}

void CanvasWidget::setZoom(double factor) {
    m_zoomFactor = qBound(0.2, factor, 5.0);
    update();
}

void CanvasWidget::resetZoom() {
    m_zoomFactor = 1.0;
    m_zoomOffset = QPointF(0, 0);
    m_canvasOffset = QPoint(0, 0);
    update();
}

// 添加非递归的泛洪填充算法
void CanvasWidget::floodFill(QPoint seedPoint) {
    QImage image = canvasImage.convertToFormat(QImage::Format_RGB32);
    QRgb oldColor = image.pixel(seedPoint);
    QRgb newColor = penColor.rgb();
    
    if (oldColor == newColor) return;

    QQueue<QPoint> queue;
    queue.enqueue(seedPoint);
    
    const int dx8[] = { -1, 0, 1, -1, 1, -1, 0, 1 };
    const int dy8[] = { -1, -1, -1, 0, 0, 1, 1, 1 };
    const int dx4[] = { -1, 0, 1, 0 };
    const int dy4[] = { 0, -1, 0, 1 };
    
    const int* dx = fillConnectivity == EightWay ? dx8 : dx4;
    const int* dy = fillConnectivity == EightWay ? dy8 : dy4;
    const int count = fillConnectivity == EightWay ? 8 : 4;

    while (!queue.isEmpty()) {
        QPoint p = queue.dequeue();
        if (image.rect().contains(p) && image.pixel(p) == oldColor) {
            image.setPixel(p, newColor);
            for (int i = 0; i < count; ++i) {
                QPoint np(p.x() + dx[i], p.y() + dy[i]);
                if (image.rect().contains(np) && image.pixel(np) == oldColor) {
                    queue.enqueue(np);
                }
            }
        }
    }
    
    canvasImage = image.convertToFormat(QImage::Format_ARGB32);
}

void CanvasWidget::setFillConnectivity(Connectivity conn) {
    fillConnectivity = conn;
}
