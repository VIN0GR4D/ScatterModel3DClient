#include "portraitwindow.h"
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDebug>
#include <algorithm>

PortraitWindow::PortraitWindow(QWidget *parent)
    : QDialog(parent), offset(0, 0), scale(1.0), maxDataValue(1.0) {
    setWindowTitle("2D Портрет");
    resize(1280, 720);

    // Инициализируем ширину легенды
    legendWidth = 60;
}

void PortraitWindow::setData(const QVector<QVector<double>> &data) {
    this->data = data;

    // Вычисляем максимальное и минимальное значения в данных для корректного отображения цветов
    maxDataValue = 0.0;
    minDataValue = std::numeric_limits<double>::max();
    for (const auto &row : data) {
        for (double val : row) {
            if (val > maxDataValue) {
                maxDataValue = val;
            }
            if (val < minDataValue) {
                minDataValue = val;
            }
        }
    }

    // Избегаем деления на ноль
    if (maxDataValue == minDataValue) {
        maxDataValue += 1.0;
        minDataValue -= 1.0;
    }

    // Автоматическое масштабирование, чтобы данные вписались в окно
    int numRows = data.size();
    int numCols = data[0].size();

    double dataWidth = numCols * 1.0;  // cellWidth = 1.0
    double dataHeight = numRows * 1.0; // cellHeight = 1.0

    // Учтем ширину легенды при вычислении доступной ширины
    double availableWidth = width() - legendWidth - 20; // 20 пикселей на отступы
    double availableHeight = height();

    double scaleX = availableWidth / dataWidth;
    double scaleY = availableHeight / dataHeight;
    scale = std::min(scaleX, scaleY);

    // Центрирование данных в окне
    double sceneWidth = dataWidth * scale;
    double sceneHeight = dataHeight * scale;
    offset.setX((availableWidth - sceneWidth) / 2 + 10); // 10 пикселей отступ слева
    offset.setY((availableHeight - sceneHeight) / 2);

    update(); // Обновляем виджет для перерисовки
}

void PortraitWindow::paintEvent(QPaintEvent *event) {
    if (data.isEmpty())
        return;

    QPainter painter(this);
    painter.save();

    // Применяем смещение и масштабирование для рисования данных
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

    // Рисуем цветовую шкалу после восстановления преобразований
    drawColorScale(painter);
}

void PortraitWindow::drawColorScale(QPainter &painter) {
    int legendHeight = height() * 0.8; // Высота легенды занимает 80% высоты окна
    int legendX = width() - legendWidth - 30; // Отступ ближе к графику
    int legendY = (height() - legendHeight) / 2; // Центрируем по Y

    // Рисуем градиентную шкалу
    QRect legendRect(legendX, legendY, legendWidth - 10, legendHeight);

    QLinearGradient gradient(legendRect.topLeft(), legendRect.bottomLeft());
    gradient.setColorAt(0.0, getColorForValue(maxDataValue));
    gradient.setColorAt(1.0, getColorForValue(minDataValue));

    painter.fillRect(legendRect, gradient);
    painter.setPen(Qt::black);
    painter.drawRect(legendRect);

    // Рисуем метки значений
    painter.setPen(Qt::black);
    QFont font = painter.font();
    font.setPointSize(10);
    painter.setFont(font);

    int numTicks = 5; // Количество меток
    for (int i = 0; i <= numTicks; ++i) {
        double ratio = static_cast<double>(i) / numTicks;
        int y = legendY + ratio * legendHeight;
        double value = maxDataValue - ratio * (maxDataValue - minDataValue);
        QString text = QString::number(value, 'g', 4);

        // Корректируем отступ для текста, чтобы он вписывался в окно
        int textX = legendX + legendWidth - 30; // Увеличиваем отступ, чтобы числа не обрезались
        painter.drawText(textX, y + 5, text);
    }
}

QColor PortraitWindow::getColorForValue(double value) const {
    double normalizedValue = (value - minDataValue) / (maxDataValue - minDataValue);
    int colorValue = static_cast<int>(normalizedValue * 255);
    colorValue = qBound(0, colorValue, 255);
    int hue = 240 - colorValue * 240 / 255; // От синего (240) к красному (0)
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
