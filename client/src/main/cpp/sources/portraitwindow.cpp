#include "portraitwindow.h"
#include <QPaintEvent>
#include <QMouseEvent>

PortraitWindow::PortraitWindow(QWidget *parent) : QDialog(parent) {
    setWindowTitle("2D Портрет");
    resize(1280, 720);
}

void PortraitWindow::setData(const QVector<QVector<double>> &data) {
    this->data = data;
    update(); // Обновляем виджет для перерисовки
}

// void PortraitWindow::paintEvent(QPaintEvent *event) {
//     if (data.isEmpty()) return;

//     QPainter painter(this);
//     int numRows = data.size();
//     int numCols = data[0].size();

//     double cellWidth = scale * width() / numCols;
//     double cellHeight = scale * height() / numRows;

//     for (int row = 0; row < numRows; ++row) {
//         for (int col = 0; col < numCols; ++col) {
//             double value = data[row][col];
//             QColor color = getColorForValue(value);
//             painter.fillRect(scale * col * cellWidth + offset.x(), scale * row * cellHeight + offset.y(), cellWidth, cellHeight, color);
//         }
//     }
// }

void PortraitWindow::paintEvent(QPaintEvent *event) {
    if (data.isEmpty()) return;

    QPainter painter(this);

    // Определение количества строк и столбцов
    int numRows = data.size();
    if (numRows == 0) return;
    int numCols = data[0].size();
    if (numCols == 0) return;

    // Вычисление размеров ячеек
    double cellWidth = scale * width() / numCols;
    double cellHeight = scale * height() / numRows;

    for (int row = 0; row < numRows; ++row) {
        // Отражение по вертикали (верх-низ)
        int flippedRow = numRows - 1 - row;

        for (int col = 0; col < numCols; ++col) {
            // Отражение по горизонтали (право-лево)
            int flippedCol = numCols - 1 - col;

            // Получение значения из отражённых координат
            double value = data[flippedRow][flippedCol];
            QColor color = getColorForValue(value);

            // Отрисовка ячейки
            painter.fillRect(scale * col * cellWidth + offset.x(),
                             scale * row * cellHeight + offset.y(),
                             cellWidth,
                             cellHeight,
                             color);
        }
    }
}

QColor PortraitWindow::getColorForValue(double value) const {
    int colorValue = static_cast<int>((value / 4.0) * 255);
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
        QPointF numSteps = numDegrees / 15.0;
        scale *= (1.0 + numSteps.y() / 10);
        update();
    }
    event->accept();
}
