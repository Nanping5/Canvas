//@author Nanping5
//@date 2025/3/24
#include "canvaswidget.h"
#include <QPainterPath>
#include<cmath>
#include <QQueue>
#include <QStack>

CanvasWidget::CanvasWidget(QWidget *parent) :
    QWidget(parent),
    penColor(Qt::black),
    penWidth(2),
    drawing(false),
    drawingMode(0),
    lineStyle(Qt::SolidLine),
    backgroundColor(Qt::white),
    m_zoomFactor(1.0),
    isFirstEdge(false),
    fillConnectivity(EightWay),
    clipAlgorithm(CohenSutherland),
    lineAlgorithm(Bresenham),
    selectionMode(0),
    isDraggingClipRect(false),
    isDraggingSelection(false),
    isSelecting(false),
    isMoving(false),
    transformMode(None),
    isRotating(false),
    rotateAngle(0),
    initialAngle(0),
    currentAngle(0),
    scaleRect(QRect()),
    scaleOriginal(QImage()),
    scaleFactor(1.0),
    isScaling(false),
    isAdjustingCurve(false),
    selectedPointIndex(-1),
    curvePreviewImage(QImage())
{
    setFocusPolicy(Qt::StrongFocus);
    canvasImage = QImage(800, 600, QImage::Format_ARGB32);
    canvasImage.fill(Qt::transparent);
    originalCanvas = canvasImage.copy();
    setMouseTracking(true);
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
    if (mode != 7) {
        controlPoints.clear(); // 切换到其他模式时清除控制点
    }
    update();
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

    // 如果正在移动，绘制预览
    if ((selectionMode == 1 || selectionMode == 2) && isMoving && !selectionImage.isNull()) {
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        // 在预览时也只显示非白色和非透明像素
        for(int y = 0; y < selectionImage.height(); ++y) {
            for(int x = 0; x < selectionImage.width(); ++x) {
                QColor pixelColor = selectionImage.pixelColor(x, y);
                if(pixelColor != Qt::white && pixelColor.alpha() > 0) {
                    painter.setPen(pixelColor);
                    painter.drawPoint(selectionRect.x() + x, selectionRect.y() + y);
                }
            }
        }
    }

    // 绘制选择框
    if ((selectionMode == 1 || selectionMode == 2) && !selectionRect.isNull()) {
        QPen dashPen(Qt::black, 1, Qt::DashLine);
        painter.setPen(dashPen);
        painter.drawRect(selectionRect);
    }

    // 绘制绘画模式的预览
    if (drawing && selectionMode == 0) {
        painter.setRenderHint(QPainter::Antialiasing);
        QPen pen(penColor, penWidth, lineStyle, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);

        switch (drawingMode) {
        case 1: // 直线
            painter.drawLine(startPoint, currentPoint);
            break;
        case 2: { // 圆
            int radius = static_cast<int>(sqrt(pow(currentPoint.x() - startPoint.x(), 2) +
                                               pow(currentPoint.y() - startPoint.y(), 2)));
            painter.drawEllipse(startPoint, radius, radius);
            break;
        }
        case 4: // 多边形
            if (!polygonPoints.isEmpty()) {
                for (int i = 1; i < polygonPoints.size(); ++i) {
                    painter.drawLine(polygonPoints[i-1], polygonPoints[i]);
                }
                // 绘制最后一条线到当前点
                painter.drawLine(polygonPoints.last(), currentPoint);

                // 如果接近起点，显示闭合预览
                if (polygonPoints.size() >= 2 &&
                    QLineF(currentPoint, firstVertex).length() < CLOSE_DISTANCE) {
                    painter.drawLine(currentPoint, firstVertex);
                }
            }
            break;
        case 8: { // 圆弧
            int radius = static_cast<int>(sqrt(pow(currentPoint.x() - startPoint.x(), 2) +
                                               pow(currentPoint.y() - startPoint.y(), 2)));
            double dx = currentPoint.x() - startPoint.x();
            double dy = currentPoint.y() - startPoint.y();
            double startAngle = qRadiansToDegrees(atan2(-dy, dx));
            drawMidpointArc(painter, startPoint - m_canvasOffset, radius, startAngle, startAngle+90, false);
            break;
        }
        }
    }

    // 绘制控制点和Bezier曲线
    if (drawingMode == 7 && !controlPoints.isEmpty()) {
        painter.setRenderHint(QPainter::Antialiasing);

        // 绘制控制点
        painter.setPen(QPen(Qt::red, 5));
        for (const QPoint &point : controlPoints) {
            painter.drawPoint(point);
        }

        // 绘制控制多边形
        painter.setPen(QPen(Qt::gray, 1, Qt::DashLine));
        for (int i = 1; i < controlPoints.size(); ++i) {
            painter.drawLine(controlPoints[i-1], controlPoints[i]);
        }

        // 绘制Bezier曲线
        if (controlPoints.size() >= 2) {
            drawBezierCurve(painter);
        }
    }

    // 绘制裁剪框预览
    if (drawingMode == 6 && isDraggingClipRect) {
        painter.setPen(QPen(Qt::red, 1, Qt::DashLine));
        painter.drawRect(QRect(clipStartPoint, currentPoint).normalized());
    }

    // 绘制当前裁剪窗口
    if (!clipRect.isNull()) {
        painter.setPen(QPen(Qt::blue, 1, Qt::DashLine));
        painter.drawRect(clipRect);
    }

    // 绘制裁剪后的线段
    painter.setPen(QPen(Qt::green, 2));
    for (const QLine& line : clippedLines) {
        painter.drawLine(mapFromCanvas(line.p1()), mapFromCanvas(line.p2()));
    }

    // 绘制裁剪后的多边形
    painter.setPen(QPen(Qt::blue, 2));
    for (const auto& poly : clippedPolygons) {
        QVector<QPoint> windowPoints;
        for (const QPoint& p : poly) {
            windowPoints.append(mapFromCanvas(p));
        }
        painter.drawPolygon(windowPoints.data(), windowPoints.size());
    }

    // 绘制原始多边形（应用画布偏移）
    painter.setPen(QPen(QColor(255,0,0,100), 2));
    for (const auto& poly : allPolygons) {
        QVector<QPoint> windowPoints;
        for (const QPoint& p : poly) {
            windowPoints.append(mapFromCanvas(p));
        }
        painter.drawPolygon(windowPoints.data(), windowPoints.size());
    }

    // 绘制旋转预览
    if (transformMode == Rotate && isRotating) {
        painter.save();
        painter.translate(m_zoomOffset);
        painter.scale(m_zoomFactor, m_zoomFactor);
        painter.translate(-m_canvasOffset);

        painter.translate(rotateCenter);
        painter.rotate(rotateAngle + currentAngle);
        painter.translate(-rotateCenter);
        painter.drawImage(m_canvasOffset, preTransformImage);

        // 绘制旋转中心标记
        painter.setPen(QPen(Qt::red, 2));
        painter.drawEllipse(rotateCenter, 5, 5);
        painter.restore();
    }

    // 绘制缩放选区
    if (transformMode == Scale) {
        painter.setPen(QPen(Qt::blue, 1, Qt::DashLine));
        painter.drawRect(scaleRect);
    }

    // 绘制可调整的贝塞尔曲线
    if (isAdjustingCurve && !controlPoints.isEmpty()) {
        QPainter previewPainter(this);
        previewPainter.setRenderHint(QPainter::Antialiasing);

        // 绘制原始图像
        previewPainter.drawImage(m_canvasOffset, curvePreviewImage);

        // 绘制控制点和连线
        QPen controlPen(Qt::blue, 2);
        previewPainter.setPen(controlPen);

        // 绘制控制点连线
        if (controlPoints.size() > 1) {
            for (int i = 0; i < controlPoints.size()-1; ++i) {
                previewPainter.drawLine(controlPoints[i], controlPoints[i+1]);
            }
        }

        // 绘制控制点
        for (const QPoint &p : controlPoints) {
            previewPainter.drawEllipse(p, 5, 5);
        }

        // 绘制当前曲线
        QPen curvePen(penColor, penWidth, lineStyle);
        previewPainter.setPen(curvePen);
        drawBezierCurve(previewPainter);
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
    if (isAdjustingCurve) {
        if (event->button() == Qt::LeftButton) {
            QPoint clickPos = mapToImage(event->pos()).toPoint();
            int minDist = 10;
            for (int i = 0; i < controlPoints.size(); ++i) {
                if (QLineF(clickPos, controlPoints[i]).length() < minDist) {
                    selectedPointIndex = i;
                    break;
                }
            }
        } else if (event->button() == Qt::RightButton) {
            // 右键结束调整并确认
            QPainter painter(&canvasImage);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(QPen(penColor, penWidth, lineStyle));
            drawBezierCurve(painter);

            isAdjustingCurve = false;
            controlPoints.clear();
            update();
            event->accept();
        }
        return;
    }

    if (drawingMode == 7) {
        if (event->button() == Qt::LeftButton) {
            QPointF imagePos = mapToImage(event->pos());
            controlPoints.append(imagePos.toPoint());
            update();
            event->accept();
        }
        return;
    }

    // 其他模式处理（放在后面）
    if (transformMode == Scale) {
        if (event->button() == Qt::LeftButton) {
            QPointF imagePos = mapToImage(event->pos());
            startPoint = imagePos.toPoint();
            scaleRect = QRect(startPoint, QSize(0,0));
            isScaling = true;
            scaleOriginal = QImage(); // 重置原始图像
        } else if (event->button() == Qt::RightButton) {
            // 右键退出缩放模式
            transformMode = None;
            scaleRect = QRect();
            update();
        }
        return;
    }
    if (transformMode == Rotate) {
        if (event->button() == Qt::LeftButton) {
            rotateCenter = mapToImage(event->pos()).toPoint();
            isRotating = true;
            preTransformImage = canvasImage.copy();

            // 计算初始角度
            QPoint initPos = mapToImage(event->pos()).toPoint();
            QPoint delta = initPos - rotateCenter;
            initialAngle = qRadiansToDegrees(atan2(delta.y(), delta.x()));
            currentAngle = 0;  // 重置当前旋转角度
        } else if (event->button() == Qt::RightButton) {
            // 右键结束旋转
            isRotating = false;
            transformMode = None;
            update();
        }
        return;
    }
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
    } else if (drawingMode == 6) { // 裁剪模式
        if (event->button() == Qt::LeftButton) {
            isDraggingClipRect = true;
            clipStartPoint = mapToImage(event->pos()).toPoint();
            clipRect.setTopLeft(clipStartPoint);
            clipRect.setBottomRight(clipStartPoint);
        } else if (event->button() == Qt::RightButton) {
            clipWindow.setBottomRight(mapToImage(event->pos()).toPoint());
            processClipping();
        }
    } else if (selectionMode == 1) { // 选择模式
        if (event->button() == Qt::LeftButton) {
            QPoint clickPos = mapToImage(event->pos()).toPoint();
            if (!selectionRect.isNull() && selectionRect.contains(clickPos)) {
                // 如果点击在选择框内，切换到移动模式
                selectionMode = 2;
                isMoving = true;
                selectionOffset = clickPos - selectionRect.topLeft();
                // 保存当前画布状态
                if (originalCanvas.isNull()) {
                    originalCanvas = canvasImage.copy();
                }
            } else {
                // 开始新的选择
                isSelecting = true;
                selectionRect = QRect();
                selectionImage = QImage();
                selectionRect.setTopLeft(clickPos);
                selectionRect.setBottomRight(clickPos);
            }
        }
    } else if (selectionMode == 2) { // 移动模式
        if (event->button() == Qt::LeftButton) {
            QPoint clickPos = mapToImage(event->pos()).toPoint();
            if (selectionRect.contains(clickPos)) {
                isMoving = true;
                selectionOffset = clickPos - selectionRect.topLeft();
                // 恢复到原始画布状态
                if (!originalCanvas.isNull()) {
                    canvasImage = originalCanvas.copy();
                }
            }
        }
    } else if (event->button() == Qt::LeftButton) {
        if (drawingMode == 7) { // Bezier曲线模式
            QPointF imagePos = mapToImage(event->pos());
            controlPoints.append(imagePos.toPoint());
            update();
        } else if (event->button() == Qt::RightButton) {
            if (controlPoints.size() >= 2) {
                // 右键确认进入调整模式
                isAdjustingCurve = true;
                curvePreviewImage = canvasImage.copy(); // 保存当前画布状态
                update();
            } else {
                // 控制点不足时清空重新开始
                controlPoints.clear();
                update();
            }
        }
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
    if (isAdjustingCurve && selectedPointIndex != -1) {
        QPoint newPos = mapToImage(event->pos()).toPoint();
        controlPoints[selectedPointIndex] = newPos;
        update();
        return;
    }

    if (transformMode == Scale && isScaling) {
        QPointF imagePos = mapToImage(event->pos());
        currentPoint = imagePos.toPoint();
        scaleRect = QRect(qMin(startPoint.x(), currentPoint.x()),
                          qMin(startPoint.y(), currentPoint.y()),
                          abs(startPoint.x() - currentPoint.x()),
                          abs(startPoint.y() - currentPoint.y()))
                        .intersected(canvasImage.rect());
        update();
        return;
    }
    if (transformMode == Rotate && isRotating) {
        QPoint currentPos = mapToImage(event->pos()).toPoint();
        QPoint delta = currentPos - rotateCenter;

        // 计算相对旋转角度
        double newAngle = qRadiansToDegrees(atan2(delta.y(), delta.x()));
        currentAngle = newAngle - initialAngle;

        update();
        return;
    }
    if (event->buttons() & Qt::MiddleButton) {
        QPoint delta = event->pos() - m_lastDragPos;
        m_zoomOffset += delta;
        m_lastDragPos = event->pos();
        update();
        event->accept();
        return;
    } else if (isDraggingClipRect && drawingMode == 6) { // 裁剪模式
        QPoint currentPoint = mapToImage(event->pos()).toPoint();
        clipRect.setBottomRight(currentPoint);
        update(); // 触发重绘
    } else if (selectionMode == 1 && isSelecting) {
        // 绘制选择框
        QPoint currentPoint = mapToImage(event->pos()).toPoint();
        selectionRect.setBottomRight(currentPoint);
        update();
    } else if ((selectionMode == 1 || selectionMode == 2) && isMoving) {
        // 恢复到原始画布状态
        canvasImage = originalCanvas.copy();

        // 移动选择框时，更新位置并绘制内容
        QPoint newPos = mapToImage(event->pos()).toPoint() - selectionOffset;
        selectionRect.moveTo(newPos);

        // 在新位置绘制选区内容
        QPainter painter(&canvasImage);
        painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
        for(int y = 0; y < selectionImage.height(); ++y) {
            for(int x = 0; x < selectionImage.width(); ++x) {
                QColor pixelColor = selectionImage.pixelColor(x, y);
                if(pixelColor != Qt::white && pixelColor.alpha() > 0) {
                    painter.setPen(pixelColor);
                    painter.drawPoint(selectionRect.x() + x, selectionRect.y() + y);
                }
            }
        }
        update();
    } else if (drawing) {
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
    if (transformMode == Scale && event->button() == Qt::LeftButton) {
        isScaling = false;
        if (scaleRect.isValid()) {
            // 保存原始选区图像
            scaleOriginal = canvasImage.copy(scaleRect);
        }
        update();
        return;
    }
    if (transformMode == Rotate && event->button() == Qt::LeftButton) {
        isRotating = false;

        // 应用旋转并累积角度
        QPainter painter(&canvasImage);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.translate(rotateCenter);
        painter.rotate(rotateAngle + currentAngle);  // 应用累积角度
        painter.translate(-rotateCenter);
        painter.drawImage(0, 0, preTransformImage);

        // 保存状态
        rotateAngle += currentAngle;  // 累积旋转角度
        preTransformImage = canvasImage.copy();
        update();
        return;
    }
    if (event->button() == Qt::MiddleButton) {
        setCursor(Qt::ArrowCursor);
        event->accept();
    } else if (event->button() == Qt::LeftButton && isDraggingClipRect) {
        isDraggingClipRect = false;
        QPoint endPoint = mapToImage(event->pos()).toPoint();
        // 确保裁剪框在图像范围内
        QPoint validStart = mapToCanvas(clipStartPoint);
        QPoint validEnd = mapToCanvas(endPoint);
        clipRect = QRect(validStart, validEnd).normalized().intersected(canvasImage.rect());
        update();
    } else if (drawingMode == 4 && event->button() == Qt::LeftButton) {
        // 检查是否接近第一个顶点
        if (QLineF(currentPoint, firstVertex).length() < CLOSE_DISTANCE &&
            polygonPoints.size() >= 3) {
            // 完成多边形绘制
            QPainter painter(&canvasImage);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(QPen(penColor, penWidth, lineStyle));
            // 转换为图像坐标系
            QVector<QPoint> imagePoints;
            for (const QPoint& p : polygonPoints) {
                imagePoints.append(mapToCanvas(p));
            }
            painter.drawPolygon(imagePoints.data(), imagePoints.size());
            allPolygons.append(imagePoints);
            drawing = false;
            polygonPoints.clear();
            update();
        }
    } else if (selectionMode == 1 && isSelecting) {
        isSelecting = false;
        selectionRect = selectionRect.normalized();

        if (!selectionRect.isNull()) {
            // 保存选择区域的图像
            selectionImage = canvasImage.copy(selectionRect);

            // 新增代码：清除原位置的选区内容
            QPainter painter(&canvasImage);
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            painter.fillRect(selectionRect, backgroundColor);

            // 保存原始画布状态（此时已清除选区内容）
            originalCanvas = canvasImage.copy();
        }
        update();
    } else if ((selectionMode == 1 || selectionMode == 2) && isMoving) {
        isMoving = false;
        update();
    } else {
        if (drawing) {
            drawing = false;
            // 处理圆弧模式的最终绘制
            if (drawingMode == 8) { // 圆弧模式
                QPointF imagePos = mapToImage(event->pos());
                endPoint = imagePos.toPoint();

                QPainter painter(&canvasImage);
                painter.setRenderHint(QPainter::Antialiasing);
                int radius = static_cast<int>(sqrt(pow(endPoint.x() - startPoint.x(), 2) +
                                                   pow(endPoint.y() - startPoint.y(), 2)));
                // 计算动态角度
                double dx = endPoint.x() - startPoint.x();
                double dy = endPoint.y() - startPoint.y();
                double startAngle = qRadiansToDegrees(atan2(-dy, dx));
                double endAngle = startAngle + 90; // 固定90度圆弧

                drawMidpointArc(painter, startPoint - m_canvasOffset, radius, startAngle, endAngle);
            }
            // 自由绘制模式不需要额外处理，因为已经实时绘制
            if (drawingMode == 1 || drawingMode == 2 || drawingMode == 3) {
                // 处理其他模式的最终绘制
                QPointF imagePos = mapToImage(event->pos());
                endPoint = imagePos.toPoint();

                QPainter painter(&canvasImage);
                painter.setRenderHint(QPainter::Antialiasing, true);

                switch (drawingMode) {
                case 1: // 直线
                    if (lineAlgorithm == Bresenham) {
                        drawBresenhamLine(painter, startPoint - m_canvasOffset, endPoint - m_canvasOffset);
                    } else if (lineAlgorithm == Midpoint) {
                        drawMidpointLine(painter, startPoint - m_canvasOffset, endPoint - m_canvasOffset);
                    }
                    break;
                case 2: { // 圆
                    int radius = static_cast<int>(sqrt(pow(endPoint.x() - startPoint.x(), 2) +
                                                       pow(endPoint.y() - startPoint.y(), 2)));
                    drawMidpointArc(painter, startPoint - m_canvasOffset, radius, 0, 0, true);
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
        if (event->button() == Qt::RightButton && drawingMode == 7 && controlPoints.size() >= 2) {
            // 右键完成Bezier曲线绘制并保存到画布
            QPainter painter(&canvasImage);
            painter.setRenderHint(QPainter::Antialiasing);
            painter.setPen(QPen(penColor, penWidth, lineStyle));
            drawBezierCurve(painter);

            // 清除控制点
            controlPoints.clear();
            update();
        }
    }

    if (event->button() == Qt::LeftButton && isAdjustingCurve) {
        selectedPointIndex = -1; // 释放选中的点
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

void CanvasWidget::drawMidpointArc(QPainter &painter, QPoint center, int radius,
                                   double startAngle, double endAngle, bool isFullCircle) {
    QPen pen(penColor, penWidth, lineStyle);
    painter.setPen(pen);

    int x = radius;
    int y = 0;
    int p = 1 - radius;

    if (isFullCircle) {
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
    } else {
        startAngle = qDegreesToRadians(startAngle);
        endAngle = qDegreesToRadians(endAngle);
        if (endAngle < startAngle) endAngle += 2*M_PI;

        auto inAngleRange = [](double angle, double start, double end) {
            angle = fmod(angle, 2*M_PI);
            if (angle < 0) angle += 2*M_PI;
            return (start <= end) ? (angle >= start && angle <= end) : (angle >= start || angle <= end);
        };

        while (x >= y) {
            // 绘制第一象限的两个关键点及其邻近点
            double baseAngle = atan2(-y, x);

            // 主点 (x,y)
            if (inAngleRange(baseAngle, startAngle, endAngle)) {
                painter.drawPoint(center.x() + x, center.y() - y);
            }

            // 对称点 (y,x)
            double symAngle = M_PI_2 - baseAngle;
            if (inAngleRange(symAngle, startAngle, endAngle)) {
                painter.drawPoint(center.x() + y, center.y() - x);
            }

            // 添加中间点提高连续性
            for (int i = 1; i <= 3; ++i) {
                double ratio = i * 0.25;
                int px = x - static_cast<int>(x * ratio);
                int py = y + static_cast<int>(y * ratio);

                if (px < 0 || py < 0) continue;

                double midAngle = atan2(-py, px);
                if (inAngleRange(midAngle, startAngle, endAngle)) {
                    painter.drawPoint(center.x() + px, center.y() - py);
                }
            }

            // 原始算法步进
            y++;
            if (p <= 0) {
                p += 2 * y + 1;
            } else {
                x--;
                p += 2 * (y - x) + 1;
            }
        }
    }
}

// 辅助函数：绘制单个圆弧点
void CanvasWidget::drawArcPoint(QPainter &painter, QPoint center,
                                int x, int y, int dashCounter, int dashLength) {
    bool drawPixel = true;

    if(lineStyle != Qt::SolidLine) {
        int cyclePos = dashCounter % (dashLength * 2);
        if(lineStyle == Qt::DashLine) {
            drawPixel = (cyclePos < dashLength);
        }
        else if(lineStyle == Qt::DotLine) {
            drawPixel = (cyclePos < 1);
        }
    }

    if(drawPixel) {
        painter.drawPoint(center.x() + x, center.y() - y);
    }
}

void CanvasWidget::wheelEvent(QWheelEvent *event) {
    if (transformMode == Scale && scaleOriginal.size().isValid()) {
        // 计算缩放增量
        double delta = event->angleDelta().y() > 0 ? 0.1 : -0.1;
        scaleFactor = qMax(0.1, scaleFactor + delta);

        // 生成缩放后的图像
        QImage scaled = scaleOriginal.scaled(
            scaleOriginal.size() * scaleFactor,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
            );

        // 计算绘制位置（居中显示）
        QPoint drawPos = scaleRect.center() - QPoint(scaled.width()/2, scaled.height()/2);

        // 应用缩放
        QPainter painter(&canvasImage);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);

        // 保存原始选区外内容
        QImage originalBackground = canvasImage.copy();

        // 清除选区内容为背景色
        painter.fillRect(scaleRect, backgroundColor);

        // 绘制缩放后的图像（保持居中）
        painter.drawImage(drawPos, scaled);

        update();
        event->accept();
    } else if (event->modifiers() & Qt::ControlModifier) {
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

enum ClipAlgorithm { CohenSutherland, MidpointSubdivision };
void CanvasWidget::setClipAlgorithm(ClipAlgorithm algo) {
    clipAlgorithm = algo;
}

QRect clipWindow; // 裁剪窗口
ClipAlgorithm clipAlgorithm = CohenSutherland;
QVector<QLine> clippedLines; // 存储裁剪后的线段

// Cohen-Sutherland算法的区域编码
const int INSIDE = 0; // 0000
const int LEFT = 1;   // 0001
const int RIGHT = 2;  // 0010
const int BOTTOM = 4; // 0100
const int TOP = 8;    // 1000

int CanvasWidget::computeOutCode(const QPoint &p) const {
    int code = INSIDE;

    if (p.x() < clipRect.left()) code |= LEFT;
    if (p.x() > clipRect.right()) code |= RIGHT;
    if (p.y() < clipRect.top()) code |= TOP;
    if (p.y() > clipRect.bottom()) code |= BOTTOM;

    return code;
}

bool CanvasWidget::cohenSutherlandClip(QLine &line) {
    QPoint p1 = line.p1();
    QPoint p2 = line.p2();

    int outcode1 = computeOutCode(p1);
    int outcode2 = computeOutCode(p2);
    bool accept = false;

    while (true) {
        if (!(outcode1 | outcode2)) { // 完全在窗口内
            accept = true;
            break;
        } else if (outcode1 & outcode2) { // 完全在窗口外
            break;
        } else {
            QPoint p;
            int outcodeOut = outcode1 ? outcode1 : outcode2;

            if (outcodeOut & TOP) {
                p.setX(p1.x() + (p2.x() - p1.x()) * (clipWindow.top() - p1.y()) / (p2.y() - p1.y()));
                p.setY(clipWindow.top());
            } else if (outcodeOut & BOTTOM) {
                p.setX(p1.x() + (p2.x() - p1.x()) * (clipWindow.bottom() - p1.y()) / (p2.y() - p1.y()));
                p.setY(clipWindow.bottom());
            } else if (outcodeOut & RIGHT) {
                p.setY(p1.y() + (p2.y() - p1.y()) * (clipWindow.right() - p1.x()) / (p2.x() - p1.x()));
                p.setX(clipWindow.right());
            } else if (outcodeOut & LEFT) {
                p.setY(p1.y() + (p2.y() - p1.y()) * (clipWindow.left() - p1.x()) / (p2.x() - p1.x()));
                p.setX(clipWindow.left());
            }

            if (outcodeOut == outcode1) {
                p1 = p;
                outcode1 = computeOutCode(p1);
            } else {
                p2 = p;
                outcode2 = computeOutCode(p2);
            }
        }
    }
    if (accept) {
        line.setP1(p1);
        line.setP2(p2);
    }
    return accept;
}

void CanvasWidget::midpointSubdivisionClip(QLine line) {
    QStack<QLine> stack;
    stack.push(line);

    while (!stack.isEmpty()) {
        QLine current = stack.pop();
        int code1 = computeOutCode(current.p1());
        int code2 = computeOutCode(current.p2());

        if (!(code1 | code2)) { // 完全可见
            clippedLines.append(current);
        } else if (!(code1 & code2)) { // 部分可见
            QPoint mid((current.p1().x() + current.p2().x()) / 2,
                       (current.p1().y() + current.p2().y()) / 2);
            stack.push(QLine(mid, current.p2()));
            stack.push(QLine(current.p1(), mid));
        }
    }
}

void CanvasWidget::processClipping() {
    // 清空之前的结果
    clippedLines.clear();
    clippedPolygons.clear();

    // 处理线段裁剪（原有逻辑）
    originalLines = getDrawnLines();
    foreach (QLine line, originalLines) {
        if (clipAlgorithm == CohenSutherland) {
            QLine clipped = line;
            if (cohenSutherlandClip(clipped)) {
                clippedLines.append(clipped);
            }
        } else {
            midpointSubdivisionClip(line);
        }
    }

    // 处理多边形裁剪（新增逻辑）
    if (!allPolygons.isEmpty()) {
        clipPolygons();
    }

    // 清除裁剪框外的内容（原有逻辑）
    QPainter painter(&canvasImage);
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.setPen(Qt::NoPen);
    painter.setBrush(backgroundColor);

    QRegion outsideRegion = QRegion(canvasImage.rect()).subtracted(QRegion(clipRect));
    painter.setClipRegion(outsideRegion);
    painter.drawRect(canvasImage.rect());

    // 保留裁剪框
    update();
}

QVector<QLine> CanvasWidget::getDrawnLines() {
    // 这里需要实现获取已绘制线段的逻辑
    // 例如，可以维护一个 QVector<QLine> 的成员变量来存储所有绘制的线段
    return QVector<QLine>(); // 返回空的 QVector 作为占位符
}

void CanvasWidget::drawMidpointLine(QPainter &painter, QPoint p1, QPoint p2) {
    QPen currentPen = painter.pen();
    currentPen.setWidth(penWidth);  // 应用当前线宽
    painter.setPen(currentPen);

    int x1 = p1.x(), y1 = p1.y();
    int x2 = p2.x(), y2 = p2.y();
    int dx = abs(x2 - x1), dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1, sy = (y1 < y2) ? 1 : -1;
    int err = dx - dy;

    while (true) {
        painter.drawPoint(x1, y1);
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void CanvasWidget::setLineAlgorithm(LineAlgorithm algo) {
    lineAlgorithm = algo;
}

void CanvasWidget::setSelectionMode(bool enabled) {
    if (enabled && selectionMode == 0) {
        // 从其他模式进入选择模式
        selectionMode = 1;
        selectionRect = QRect();
        selectionImage = QImage();
        originalCanvas = QImage();
        isSelecting = false;
        isMoving = false;
    } else {
        // 退出选择模式，将当前状态合并到画布
        selectionMode = 0;
        selectionRect = QRect();
        selectionImage = QImage();
        originalCanvas = QImage();
        isSelecting = false;
        isMoving = false;
    }
    update();
}

// 实现de Casteljau算法
QPoint CanvasWidget::deCasteljau(const QVector<QPoint> &points, double t) {
    // 正确实现de Casteljau算法
    QVector<QPoint> temp = points;
    while (temp.size() > 1) {
        QVector<QPoint> newLevel;
        for (int i = 0; i < temp.size() - 1; ++i) {
            int x = (1 - t) * temp[i].x() + t * temp[i + 1].x();
            int y = (1 - t) * temp[i].y() + t * temp[i + 1].y();
            newLevel.append(QPoint(x, y));
        }
        temp = newLevel;
    }
    return temp.first();
}

// 绘制Bezier曲线
void CanvasWidget::drawBezierCurve(QPainter &painter) {
    if (controlPoints.size() < 2) return;

    // 使用de Casteljau算法
    QVector<QPoint> points;
    const double step = 0.01; // 更精细的步长
    for (double t = 0; t <= 1.0; t += step) {
        points.append(deCasteljau(controlPoints, t));
    }

    painter.drawPolyline(points.data(), points.size());
}

// 实现保存函数
bool CanvasWidget::saveImage(const QString &fileName, const char *format) {
    // 创建一个与画布大小相同的临时图像
    QImage image(canvasImage.size(), QImage::Format_ARGB32);
    image.fill(Qt::white);

    // 将当前画布内容绘制到临时图像上
    QPainter painter(&image);
    painter.drawImage(0, 0, canvasImage);

    // 如果当前正在绘制Bezier曲线，也将其绘制到图像上
    if (drawingMode == 7 && !controlPoints.isEmpty()) {
        painter.setPen(QPen(penColor, penWidth, lineStyle));
        drawBezierCurve(painter);
    }

    // 保存图像
    return image.save(fileName, format);
}

void CanvasWidget::clipPolygons() {
    clippedPolygons.clear();

    if (clipRect.isEmpty() || allPolygons.isEmpty()) return;

    // 使用图像坐标系进行裁剪计算
    QRect imageClipRect = QRect(
                              mapToCanvas(clipRect.topLeft()),
                              mapToCanvas(clipRect.bottomRight())
                              ).normalized();

    int xmin = imageClipRect.left();
    int ymin = imageClipRect.top();
    int xmax = imageClipRect.right();
    int ymax = imageClipRect.bottom();

    for (const auto& polygon : allPolygons) {
        if (polygon.size() < 3) continue; // 忽略无效多边形

        QVector<QPoint> outputList = polygon;

        for (int edge = 0; edge < 4; edge++) {
            QVector<QPoint> inputList = outputList;
            outputList.clear();

            if (inputList.isEmpty()) break;

            QPoint s = inputList.last();
            for (const QPoint& p : inputList) {
                if (inside(p, edge, xmin, ymin, xmax, ymax)) {
                    if (!inside(s, edge, xmin, ymin, xmax, ymax)) {
                        QPoint intersect = computeIntersection(s, p, edge, xmin, ymin, xmax, ymax);
                        if (!intersect.isNull()) {
                            outputList.append(computeIntersection(s, p, edge, xmin, ymin, xmax, ymax));
                        }
                    }
                    outputList.append(p);
                } else if (inside(s, edge, xmin, ymin, xmax, ymax)) {
                    QPoint intersect = computeIntersection(s, p, edge, xmin, ymin, xmax, ymax);
                    if (!intersect.isNull()) {
                        outputList.append(computeIntersection(s, p, edge, xmin, ymin, xmax, ymax));
                    }
                }
                s = p;
            }
        }

        if (outputList.size() >= 3) { // 只保留有效多边形
            clippedPolygons.append(outputList);
        }
    }
}

// 辅助函数：判断点是否在边界内侧
bool CanvasWidget::inside(const QPoint& p, int edge, int xmin, int ymin, int xmax, int ymax) {
    switch (edge) {
    case 0: return p.x() >= xmin; // left
    case 1: return p.y() <= ymax; // bottom
    case 2: return p.x() <= xmax; // right
    case 3: return p.y() >= ymin; // top
    }
    return false;
}

// 计算线段与边界的交点
QPoint CanvasWidget::computeIntersection(QPoint p1, QPoint p2, int edge,
                                         int xmin, int ymin, int xmax, int ymax) {
    // 处理垂直线段
    if (qFuzzyCompare(static_cast<double>(p1.x()), static_cast<double>(p2.x()))) {
        if (edge == 0 || edge == 2) { // 左右边界
            return QPoint(); // 无效交点
        }
        // 处理水平边界
        double y = (edge == 1) ? ymax : ymin;
        return QPoint(p1.x(), y);
    }

    // 处理水平线段
    if (qFuzzyCompare(static_cast<double>(p1.y()), static_cast<double>(p2.y()))) {
        if (edge == 1 || edge == 3) { // 上下边界
            return QPoint(); // 无效交点
        }
        // 处理垂直边界
        double x = (edge == 0) ? xmin : xmax;
        return QPoint(x, p1.y());
    }

    double m = (p2.y() - p1.y()) / static_cast<double>(p2.x() - p1.x());

    double x, y;

    switch (edge) {
    case 0: // left
        x = xmin;
        y = m * (xmin - p1.x()) + p1.y();
        break;
    case 1: // bottom
        y = ymax;
        x = (ymax - p1.y()) / m + p1.x();
        break;
    case 2: // right
        x = xmax;
        y = m * (xmax - p1.x()) + p1.y();
        break;
    case 3: // top
        y = ymin;
        x = (ymin - p1.y()) / m + p1.x();
        break;
    }

    // 检查交点是否在线段范围内
    if (x < qMin(p1.x(), p2.x()) || x > qMax(p1.x(), p2.x()) ||
        y < qMin(p1.y(), p2.y()) || y > qMax(p1.y(), p2.y())) {
        return QPoint(); // 无效交点
    }

    return QPoint(round(x), round(y));
}

void CanvasWidget::confirmClipping() {
    if (!clipRect.isValid()) return;

    QPainter painter(&canvasImage);

    // 转换为图像坐标系（考虑画布偏移）
    QRect imageClipRect = QRect(
                              mapToCanvas(clipRect.topLeft() - m_canvasOffset),
                              mapToCanvas(clipRect.bottomRight() - m_canvasOffset)
                              ).normalized().intersected(canvasImage.rect());

    // 清除外部区域（仅清除裁剪框外内容）
    painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.setPen(Qt::NoPen);
    painter.setBrush(backgroundColor);
    painter.setClipRegion(QRegion(canvasImage.rect()).subtracted(imageClipRect));
    painter.drawRect(canvasImage.rect());

    // 绘制裁剪后的图形到画布（使用原始坐标）
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.setClipRect(imageClipRect);

    // 绘制线段
    painter.setPen(QPen(penColor, penWidth));
    for (const QLine& line : clippedLines) {
        painter.drawLine(line);
    }

    // 绘制多边形（使用图像坐标系）
    for (const auto& poly : clippedPolygons) {
        painter.drawPolygon(poly.constData(), poly.size());
    }

    // 保留原始图像内容
    painter.drawImage(imageClipRect, canvasImage.copy(imageClipRect));

    // 重置状态
    clipRect = QRect();
    clippedLines.clear();
    clippedPolygons.clear();
    allPolygons.clear();

    update();
    emit clippingConfirmed();
}

void CanvasWidget::keyPressEvent(QKeyEvent *event) {
    // 优先处理裁剪确认
    if ((event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) && !clipRect.isNull()) {
        confirmClipping();
        event->accept();
        return;
    }

    // 处理贝塞尔曲线模式
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (drawingMode == 7) {  // 仅在贝塞尔曲线模式下处理
            if (!isAdjustingCurve && controlPoints.size() >= 2) {
                // 进入调整模式
                isAdjustingCurve = true;
                curvePreviewImage = canvasImage.copy();
                update();
                event->accept();
            } else if (isAdjustingCurve) {
                // 确认最终曲线
                QPainter painter(&canvasImage);
                painter.setRenderHint(QPainter::Antialiasing);
                painter.setPen(QPen(penColor, penWidth, lineStyle));
                drawBezierCurve(painter);

                // 重置状态
                isAdjustingCurve = false;
                controlPoints.clear();
                update();
                event->accept();
            }
            return;
        }
    }

    // 处理ESC键（仅影响调整模式）
    if (event->key() == Qt::Key_Escape) {
        if (isAdjustingCurve) {
            // 取消曲线调整
            isAdjustingCurve = false;
            controlPoints.clear();
            update();
            event->accept();
            return;
        }
        // 可以添加其他ESC键功能
    }

    // 其他按键处理
    QWidget::keyPressEvent(event);
    setFocus(); // 确保控件获得焦点
}

// 新增函数：设置变换模式
void CanvasWidget::setTransformMode(TransformMode mode) {
    transformMode = mode;
    selectionMode = 0; // 退出选择模式
    drawingMode = -1;  // 退出其他绘制模式
    if (mode != Scale) {
        scaleRect = QRect(); // 退出时清除缩放选区
    }
    update();
}
