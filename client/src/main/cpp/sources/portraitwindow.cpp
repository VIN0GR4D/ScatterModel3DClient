#include "portraitwindow.h"
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QDebug>
#include <QFileDialog>
#include <algorithm>
#include <QMessageBox>

PortraitWindow::PortraitWindow(QWidget *parent)
    : QDialog(parent), offset(0, 0), scale(1.0), maxDataValue(1.0), isLogarithmicScale(false) {
    setWindowTitle("2D Портрет");
    resize(1280, 720);

    // Инициализируем ширину легенды
    legendWidth = 60;

    // Создаем кнопку сохранения изображения
    saveButton = new QPushButton("Сохранить изображение", this);
    saveButton->setMinimumWidth(200);
    saveButton->move(10, 10); // Позиция кнопки в левом верхнем углу
    connect(saveButton, &QPushButton::clicked, this, &PortraitWindow::savePortraitAsPNG);

    // Кнопка переключения масштаба
    toggleScaleButton = new QPushButton("Логарифмический масштаб", this);
    toggleScaleButton->setMinimumWidth(200);
    toggleScaleButton->setCheckable(true);
    toggleScaleButton->move(10, 50);
    connect(toggleScaleButton, &QPushButton::clicked, this, &PortraitWindow::toggleScale);
}

void PortraitWindow::toggleScale() {
    isLogarithmicScale = toggleScaleButton->isChecked();
    toggleScaleButton->setText(isLogarithmicScale ? "Обычный масштаб" : "Логарифмический масштаб");
    update();
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
    painter.scale(scale, -scale); // Инвертируем ось Y

    // Корректируем смещение по Y для компенсации инвертированной оси
    painter.translate(0, -data.size());

    // Рисуем данные
    drawData(painter);

    painter.restore();

    // Рисуем цветовую шкалу после восстановления преобразований
    drawColorScale(painter);
}

void PortraitWindow::drawData(QPainter &painter) {
    int numRows = data.size();
    int numCols = data[0].size();

    for (int row = 0; row < numRows; ++row) {
        for (int col = 0; col < numCols; ++col) {
            double value = data[row][col];
            QColor color = getColorForValue(value);
            painter.fillRect(col * 1.0, row * 1.0, 1.0, 1.0, color);
        }
    }
}

void PortraitWindow::drawColorScale(QPainter &painter) {
    int legendHeight = static_cast<int>(height() * 0.8);
    int legendX = width() - legendWidth - 30;
    int legendY = (height() - legendHeight) / 2;

    QRect legendRect(legendX, legendY, legendWidth - 10, legendHeight);
    QLinearGradient gradient(legendRect.bottomLeft(), legendRect.topLeft());

    const int numStops = 100;
    for (int i = 0; i <= numStops; ++i) {
        double ratio = static_cast<double>(i) / numStops;
        double value;
        if (isLogarithmicScale) {
            double minPositive = std::max(minDataValue, 1e-10);
            double logMin = std::log10(minPositive);
            double logMax = std::log10(std::max(maxDataValue, minPositive * 1.1));
            value = std::pow(10.0, logMin + ratio * (logMax - logMin));
        } else {
            value = minDataValue + ratio * (maxDataValue - minDataValue);
        }
        QColor color = getColorForValue(value);
        gradient.setColorAt(ratio, color);
    }

    painter.fillRect(legendRect, gradient);
    painter.setPen(Qt::black);
    painter.drawRect(legendRect);

    painter.setPen(Qt::black);
    QFont font = painter.font();
    font.setPointSize(10);
    painter.setFont(font);

    int numTicks = 5;
    for (int i = 0; i <= numTicks; ++i) {
        double ratio = static_cast<double>(i) / numTicks;
        int y = legendY + (1.0 - ratio) * legendHeight;
        double value;
        if (isLogarithmicScale) {
            double minPositive = std::max(minDataValue, 1e-10);
            double logMin = std::log10(minPositive);
            double logMax = std::log10(std::max(maxDataValue, minPositive * 1.1));
            value = std::pow(10.0, logMin + ratio * (logMax - logMin));
        } else {
            value = minDataValue + ratio * (maxDataValue - minDataValue);
        }
        QString text = QString::number(value, 'g', 4);
        int textX = legendX + legendWidth - 30;
        painter.drawText(textX, y + 5, text);
    }
}

QColor PortraitWindow::getColorForValue(double value) const {
    if (maxDataValue == minDataValue) {
        return QColor::fromHsv(240, 255, 255);
    }

    double normalizedValue;
    if (isLogarithmicScale) {
        // Защита от отрицательных и нулевых значений в логарифмическом масштабе
        double minPositive = std::max(minDataValue, 1e-10);
        double logMin = std::log10(minPositive);
        double logMax = std::log10(std::max(maxDataValue, minPositive * 1.1));
        double logVal = std::log10(std::max(value, minPositive));

        normalizedValue = (logVal - logMin) / (logMax - logMin);
    } else {
        normalizedValue = (value - minDataValue) / (maxDataValue - minDataValue);
    }

    normalizedValue = qBound(0.0, normalizedValue, 1.0);
    int hue = static_cast<int>(240 - normalizedValue * 240);
    hue = qBound(0, hue, 359);

    return QColor::fromHsv(hue, 255, 255);
}

void PortraitWindow::savePortraitAsPNG() {
    // Создаем изображение с желаемым размером
    QSize imageSize(1280, 720); // Или другой размер по необходимости
    QPixmap pixmap(imageSize);
    pixmap.fill(Qt::white); // Заполняем белым фоном

    QPainter painter(&pixmap);

    // Настраиваем масштаб и смещение так, чтобы данные вписались в изображение
    int numRows = data.size();
    int numCols = data[0].size();

    double dataWidth = numCols * 1.0;  // cellWidth = 1.0
    double dataHeight = numRows * 1.0; // cellHeight = 1.0

    // Учтем ширину легенды при вычислении доступной ширины
    double availableWidth = imageSize.width() - legendWidth - 20; // 20 пикселей на отступы
    double availableHeight = imageSize.height();

    double scaleX = availableWidth / dataWidth;
    double scaleY = availableHeight / dataHeight;
    double imageScale = std::min(scaleX, scaleY);

    // Центрирование данных в изображении
    double sceneWidth = dataWidth * imageScale;
    double sceneHeight = dataHeight * imageScale;
    QPointF imageOffset;
    imageOffset.setX((availableWidth - sceneWidth) / 2 + 10); // 10 пикселей отступ слева
    imageOffset.setY((availableHeight - sceneHeight) / 2);

    painter.save();
    painter.translate(imageOffset);
    painter.scale(imageScale, -imageScale); // Инвертируем ось Y
    painter.translate(0, -data.size());

    // Рисуем данные
    drawData(painter);

    painter.restore();

    // Рисуем цветовую шкалу
    int legendHeight = static_cast<int>(imageSize.height() * 0.8); // 80% от высоты изображения
    int legendX = imageSize.width() - legendWidth - 30; // Отступ ближе к графику
    int legendY = (imageSize.height() - legendHeight) / 2; // Центрируем по Y

    QRect legendRect(legendX, legendY, legendWidth - 10, legendHeight);

    // Инвертированный градиент: от нижней к верхней части
    QLinearGradient gradient(legendRect.bottomLeft(), legendRect.topLeft());

    // Добавляем множество цветовых остановок для плавного перехода
    const int numStops = 100; // Количество цветовых остановок
    for (int i = 0; i <= numStops; ++i) {
        double ratio = static_cast<double>(i) / numStops;
        double value = minDataValue + ratio * (maxDataValue - minDataValue);
        QColor color = getColorForValue(value);
        gradient.setColorAt(ratio, color);
    }

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
        // Инвертируем позицию y
        int y = legendY + (1.0 - ratio) * legendHeight;
        double value = minDataValue + ratio * (maxDataValue - minDataValue);
        QString text = QString::number(value, 'g', 4);

        // Корректируем отступ для текста, чтобы он вписывался в изображение
        int textX = legendX + legendWidth - 30; // Увеличиваем отступ, чтобы числа не обрезались
        painter.drawText(textX, y + 5, text);
    }

    // Сохраняем изображение
    QString filename = QFileDialog::getSaveFileName(this, tr("Сохранить изображение"), "", tr("PNG Image (*.png)"));
    if (!filename.isEmpty()) {
        bool success = pixmap.save(filename);
        if (success) {
            QMessageBox::information(this, tr("Успех"), tr("Изображение успешно сохранено."));
        } else {
            QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось сохранить изображение."));
        }
    }
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
