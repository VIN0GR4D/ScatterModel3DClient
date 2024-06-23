#include "graphwindow.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QCheckBox>

GraphWindow::GraphWindow(QWidget *parent) : QDialog(parent) {
    customPlot = new QCustomPlot(this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(customPlot);

    // Оси
    customPlot->xAxis->setLabel("Index");
    customPlot->yAxis->setLabel("Value");

    // Добавляем графики для absEout и normEout
    customPlot->addGraph(); // График для absEout
    customPlot->addGraph(); // График для normEout

    // Устанавливаем цвета для графиков
    QPen absEoutPen(Qt::blue); // Синий цвет для absEout
    QPen normEoutPen(Qt::red); // Красный цвет для normEout
    customPlot->graph(0)->setPen(absEoutPen);
    customPlot->graph(1)->setPen(normEoutPen);

    // Включаем масштабирование колесиком мыши
    customPlot->setInteraction(QCP::iRangeZoom, true);
    customPlot->setInteraction(QCP::iRangeDrag, true);
    customPlot->axisRect()->setRangeZoom(Qt::Horizontal | Qt::Vertical);  // Масштабирование по обеим осям
    customPlot->axisRect()->setRangeDrag(Qt::Horizontal | Qt::Vertical);  // Перемещение по обеим осям

    absEoutCheckBox = new QCheckBox("Показать absEout", this);
    absEoutCheckBox->setChecked(true);
    connect(absEoutCheckBox, &QCheckBox::toggled, this, &GraphWindow::toggleAbsEout);
    layout->addWidget(absEoutCheckBox);

    normEoutCheckBox = new QCheckBox("Показать normEout", this);
    normEoutCheckBox->setChecked(true);
    connect(normEoutCheckBox, &QCheckBox::toggled, this, &GraphWindow::toggleNormEout);
    layout->addWidget(normEoutCheckBox);

    // Добавляем кнопку сброса масштаба
    QPushButton *resetZoomButton = new QPushButton("Сбросить до исходного положения", this);
    connect(resetZoomButton, &QPushButton::clicked, this, &GraphWindow::resetZoom);
    layout->addWidget(resetZoomButton);

    // Задаем размер окна
    this->resize(800, 600);
}

void GraphWindow::setData(const QVector<double> &x, const QVector<double> &absY, const QVector<double> &normY) {
    customPlot->graph(0)->setData(x, absY);
    customPlot->graph(1)->setData(x, normY);

    customPlot->rescaleAxes();

    // Сохраняем начальные диапазоны осей после вызова rescaleAxes
    xRange = customPlot->xAxis->range();
    yRange = customPlot->yAxis->range();

    customPlot->replot();

    // Для отладки
    qDebug() << "Graph data X:" << x;
    qDebug() << "Graph data absY:" << absY;
    qDebug() << "Graph data normY:" << normY;
}

void GraphWindow::toggleAbsEout(bool checked) {
    customPlot->graph(0)->setVisible(checked);
    customPlot->replot();
}

void GraphWindow::toggleNormEout(bool checked) {
    customPlot->graph(1)->setVisible(checked);
    customPlot->replot();
}

void GraphWindow::resetZoom() {
    customPlot->xAxis->setRange(xRange);
    customPlot->yAxis->setRange(yRange);
    customPlot->replot();
}
