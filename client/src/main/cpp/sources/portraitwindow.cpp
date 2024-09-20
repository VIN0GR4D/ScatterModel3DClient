#include "portraitwindow.h"
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDebug>

PortraitWindow::PortraitWindow(QWidget *parent)
    : QDialog(parent), offset(0, 0), scale(1.0), maxDataValue(1.0) {
    setWindowTitle("2D Портрет");
    resize(1280, 720);
}

void PortraitWindow::setData(const QVector<QVector<double>> &data) {
    this->data = data;

    // Вычисляем максимальное значение в данных для корректного отображения цветов
    maxDataValue = 0.0;
    for (const auto &row : data) {
        for (double val : row) {
            if (val > maxDataValue) {
                maxDataValue = val;
            }
        }
    }

    // Избегаем деления на ноль
    if (maxDataValue == 0.0) {
        maxDataValue = 1.0;
    }

    update(); // Обновляем виджет для перерисовки
}

void PortraitWindow::paintEvent(QPaintEvent *event) {
    if (data.isEmpty())
        return;

    QPainter painter(this);
    painter.save();

    // Применяем смещение и масштабирование
    painter.translate(offset);
    painter.scale(scale, scale);

    int numRows = data.size();
    int numCols = data[0].size();

    // Вычисляем размеры ячеек в сценических координатах
    double cellWidth = 1.0;
    double cellHeight = 1.0;

    // Рисуем данные
    for (int row = 0; row < numRows; ++row) {
        for (int col = 0; col < numCols; ++col) {
            double value = data[row][col];
            QColor color = getColorForValue(value);
            painter.fillRect(col * cellWidth, row * cellHeight, cellWidth, cellHeight, color);
        }
    }

    painter.restore();
}

QColor PortraitWindow::getColorForValue(double value) const {
    int colorValue = static_cast<int>((value / maxDataValue) * 255);
    colorValue = qBound(0, colorValue, 255);
    int hue = 240 - colorValue;
    hue = qBound(0, hue, 359);
    return QColor::fromHsv(hue, 255, 255);
}

void PortraitWindow::mousePressEvent(QMouseEvent *event) {
    lastMousePos = event->pos();
}

void PortraitWindow::mouseMoveEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        QPointF delta = event->pos() - lastMousePos;
        offset += delta;
        lastMousePos = event->pos();
        update();
    }
}

void PortraitWindow::wheelEvent(QWheelEvent *event) {
    QPoint numDegrees = event->angleDelta() / 8;
    if (!numDegrees.isNull()) {
        double factor = std::pow(1.0015, numDegrees.y());
        QPointF cursorPos = event->position();
        QPointF scenePos = (cursorPos - offset) / scale;

        scale *= factor;

        offset = cursorPos - scenePos * scale;

        update();
    }
    event->accept();
}
