#include "heatmapwindow.h"
#include <QPainter>
#include <QDebug>
#include <algorithm>
#include <cmath>

HeatmapWindow::HeatmapWindow(QWidget *parent)
    : QMainWindow(parent) {
    setWindowTitle("Тепловая карта");
    resize(800, 600);
}

void HeatmapWindow::setData(const QVector<QVector<double>> &data) {
    this->data = data;
    update(); // Перерисовать окно
}

void HeatmapWindow::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    drawHeatmap(painter);
}

void HeatmapWindow::drawHeatmap(QPainter &painter) {
    if (data.isEmpty())
        return;

    int rows = data.size();
    int cols = data[0].size();
    int cellWidth = width() / cols;
    int cellHeight = height() / rows;

    double minVal = data[0][0];
    double maxVal = data[0][0];
    for (const auto &row : data) {
        for (double val : row) {
            minVal = std::min(minVal, val);
            maxVal = std::max(maxVal, val);
        }
    }

    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            double normalizedVal = (data[i][j] - minVal) / (maxVal - minVal);
            int colorVal = static_cast<int>(normalizedVal * 240); // Use 0 to 240 for Hue
            QColor color = QColor::fromHsv(colorVal, 255, 255); // Ensure Hue is in [0, 240]
            painter.fillRect(j * cellWidth, i * cellHeight, cellWidth, cellHeight, color);
        }
    }

    drawLegend(painter, minVal, maxVal); // Добавьте эту строку
}

void HeatmapWindow::drawLegend(QPainter &painter, double minVal, double maxVal) {
    int legendHeight = 20;
    int legendWidth = width() - 100;
    int legendX = 50;
    int legendY = height() - legendHeight - 30;

    for (int i = 0; i < legendWidth; ++i) {
        double value = minVal + (maxVal - minVal) * (static_cast<double>(i) / legendWidth);
        double normalizedVal = (value - minVal) / (maxVal - minVal);
        int colorVal = static_cast<int>(normalizedVal * 240);
        QColor color = QColor::fromHsv(colorVal, 255, 255);
        painter.setPen(color);
        painter.drawLine(legendX + i, legendY, legendX + i, legendY + legendHeight);
    }

    painter.setPen(Qt::black);
    painter.drawRect(legendX, legendY, legendWidth, legendHeight);
    painter.drawText(legendX - 40, legendY + legendHeight / 2 + 5, QString::number(minVal));
    painter.drawText(legendX + legendWidth + 10, legendY + legendHeight / 2 + 5, QString::number(maxVal));
    painter.drawText(legendX + legendWidth / 2 - 20, legendY + legendHeight + 20, "Значения");
}
