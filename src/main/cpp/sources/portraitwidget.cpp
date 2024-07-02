#include "portraitwidget.h"
#include <QVBoxLayout>

PortraitWidget::PortraitWidget(QWidget *parent) : QWidget(parent) {
    customPlot = new QCustomPlot(this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(customPlot);

    customPlot->xAxis->setLabel("Angle");
    customPlot->yAxis->setLabel("Azimuth");
    customPlot->axisRect()->setupFullAxesBox(true);

    colorMap = new QCPColorMap(customPlot->xAxis, customPlot->yAxis);
    colorMap->data()->setSize(100, 100); // Размер данных 100x100
    colorMap->data()->setRange(QCPRange(0, 10), QCPRange(0, 10)); // Диапазон данных

    // Создаем и добавляем цветовую шкалу
    QCPColorScale *colorScale = new QCPColorScale(customPlot);
    colorScale->setType(QCPAxis::atRight);
    colorMap->setColorScale(colorScale);
    customPlot->plotLayout()->addElement(0, 1, colorScale); // Добавляем цветовую шкалу справа

    colorMap->setGradient(QCPColorGradient::gpPolar); // Устанавливаем градиент цвета
    colorMap->rescaleDataRange(true);

    customPlot->rescaleAxes();
}

void PortraitWidget::clearData() {
    colorMap->data()->clear(); // Очищаем данные в colorMap
    customPlot->replot(); // Перерисовываем график
}

void PortraitWidget::setData(const QVector<double> &x, const QVector<double> &y, const QVector<double> &z) {
    int dataSize = qMin(x.size(), y.size());
    dataSize = qMin(dataSize, z.size());

    for (int i = 0; i < dataSize; ++i) {
        int xi = static_cast<int>(x[i]);
        int yi = static_cast<int>(y[i]);
        colorMap->data()->setCell(xi, yi, z[i]);
    }

    colorMap->rescaleDataRange();
    customPlot->rescaleAxes();
    customPlot->replot();
}
