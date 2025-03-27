#include "canvaswidget.h"
#include <QPainterPath>
#include<cmath>
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
    
    // 只在必要时计算和扩展画布
    if (!event->rect().contains(rect())) {
        // 计算当前视口在画布坐标系中的范围
        QPointF topLeft = mapToImage(QPoint(0, 0));
        QPointF bottomRight = mapToImage(QPoint(width(), height()));
        QRectF viewportRect = QRectF(topLeft, bottomRight).normalized();
        
        // 检查是否需要扩展画布（添加最大尺寸限制）
        const int MaxCanvasSize = 10000; // 设置最大画布尺寸
        QRect imageRect = canvasImage.rect();
        QRect newRect = imageRect;
        
        if (!imageRect.contains(viewportRect.toRect())) {
            // 计算新的画布大小，确保不超过最大限制
            QRect expandedRect = imageRect.united(viewportRect.toRect());
            newRect = QRect(
                qBound(-MaxCanvasSize, expandedRect.left(), MaxCanvasSize),
                qBound(-MaxCanvasSize, expandedRect.top(), MaxCanvasSize),
                qMin(MaxCanvasSize * 2, expandedRect.width()),
                qMin(MaxCanvasSize * 2, expandedRect.height())
            );
            
            // 只有当新尺寸确实更大时才扩展
            if (newRect != imageRect) {
                QImage newImage(newRect.size(), QImage::Format_ARGB32);
                newImage.fill(backgroundColor);
                
                // 计算原始内容的位置
                QPoint contentPos(
                    qMax(0, imageRect.x() - newRect.x()),
                    qMax(0, imageRect.y() - newRect.y())
                );
                
                // 绘制原始内容到新画布
                QPainter p(&newImage);
                p.drawImage(contentPos, canvasImage);
                
                // 更新偏移量
                QPoint oldOffset = m_canvasOffset;
                m_canvasOffset = -newRect.topLeft();
                
                // 更新缩放偏移以保持视图位置
                m_zoomOffset += (m_canvasOffset - oldOffset) * m_zoomFactor;
                
                canvasImage = newImage;
            }
        }
    }
    
    // 应用变换
    painter.translate(m_zoomOffset);
    painter.scale(m_zoomFactor, m_zoomFactor);
    painter.translate(-m_canvasOffset); // 移动到画布坐标系
    
    // 绘制画布
    painter.drawImage(m_canvasOffset, canvasImage);
    
    // 绘制实时预览
    if (drawing) {
        QColor previewColor;
        float scaledWidth = penWidth * m_zoomFactor;
        
        if (drawingMode == 3) {
            previewColor = Qt::gray;
        } else {
            previewColor = penColor;
            previewColor.setAlpha(128);
        }
        painter.setPen(QPen(previewColor, scaledWidth, Qt::SolidLine));
        painter.setRenderHint(QPainter::Antialiasing);

        // 在画布坐标系中绘制预览
        switch (drawingMode) {
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

void CanvasWidget::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        m_lastDragPos = event->pos();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
    } else {
        QPointF imagePos = mapToImage(event->pos());
        startPoint = imagePos.toPoint();
        endPoint = startPoint;
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
    }
    
    if (drawing) {
        QPointF imagePos = mapToImage(event->pos());
        currentPoint = imagePos.toPoint();
        
        if (drawingMode == 0) { // 自由绘制模式
            QPainter painter(&canvasImage);
            painter.setRenderHint(QPainter::Antialiasing);
            
            // 使用原始画笔宽度
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
        else if (drawingMode == 3) { // 橡皮擦模式
            QPainter painter(&canvasImage);
            // 使用原始画笔宽度
            painter.setPen(QPen(backgroundColor, penWidth, Qt::SolidLine, Qt::RoundCap));
            painter.drawLine(startPoint - m_canvasOffset, currentPoint - m_canvasOffset);
            startPoint = currentPoint;
        }
        update();
    }
}

void CanvasWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::MiddleButton) {
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else {
        if (drawing) {
            drawing = false;
            // 转换坐标到画布坐标系
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
            update();
        }
    }
}

void CanvasWidget::drawBresenhamLine(QPainter &painter, QPoint p1, QPoint p2) {
    // 使用原始画笔宽度
    QPen pen(penColor, penWidth, Qt::SolidLine);
    painter.setPen(pen);
    
    // 转换为当前画布坐标系
    p1 -= m_canvasOffset;
    p2 -= m_canvasOffset;
    
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
    // 使用原始画笔宽度
    QPen pen(penColor, penWidth);
    painter.setPen(pen);
    
    // 转换为当前画布坐标系
    center -= m_canvasOffset;
    
    int x = radius;
    int y = 0;
    int p = 1 - radius;

    while (x >= y) {
        bool drawPixel = true;
        if(lineStyle != Qt::SolidLine) {
            if(lineStyle == Qt::DotLine && lineStyle == Qt::DotLine) {
                drawPixel = false;
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
