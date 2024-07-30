#include "portraitwindow.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QVector>
#include "qcustomplot.h"  // Подключаем QCustomPlot для отрисовки

PortraitWindow::PortraitWindow(QWidget *parent) :
    QMainWindow(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    customPlot = new QCustomPlot(this);
    layout->addWidget(customPlot);

    QWidget *widget = new QWidget();
    widget->setLayout(layout);
    setCentralWidget(widget);

    customPlot->xAxis->setLabel("X");
    customPlot->yAxis->setLabel("Amplitude");
}

void PortraitWindow::setData(const QVector<QVector<double>> &absEout, const QVector<QVector<double>> &normEout) {
    customPlot->clearGraphs();
    int numGraphs = absEout.size();

    for (int i = 0; i < numGraphs; ++i) {
        customPlot->addGraph();
        QVector<double> x(absEout[i].size()), y(absEout[i].size());
        for (int j = 0; j < absEout[i].size(); ++j) {
            x[j] = j;  // X координата - индекс точки
            y[j] = absEout[i][j];  // Y координата - значение
        }
        customPlot->graph(i)->setData(x, y);
        qDebug() << "Graph" << i << "X:" << x << "Y:" << y;  // Debug вывод данных для каждой линии
    }

    customPlot->rescaleAxes();
    customPlot->replot();
}

void PortraitWindow::clearData() {
    customPlot->clearGraphs();
    customPlot->replot();
}
