#include "graphwindow.h"
#include <QVBoxLayout>

GraphWindow::GraphWindow(QWidget *parent) : QDialog(parent) {
    customPlot = new QCustomPlot(this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(customPlot);

    customPlot->xAxis->setLabel("Index");
    customPlot->yAxis->setLabel("Value");
    customPlot->addGraph();

    // Задаем размер окна
    this->resize(800, 600);  // Установите размер, который считаете подходящим

}

void GraphWindow::setData(const QVector<double> &x, const QVector<double> &y) {
    customPlot->graph(0)->setData(x, y);
    customPlot->rescaleAxes();
    customPlot->replot();
}
