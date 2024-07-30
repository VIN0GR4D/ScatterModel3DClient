#ifndef PORTRAITWINDOW_H
#define PORTRAITWINDOW_H

#include <QMainWindow>
#include <QVector>
#include "qcustomplot.h"  // Подключаем QCustomPlot для отрисовки

class PortraitWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PortraitWindow(QWidget *parent = nullptr);
    void setData(const QVector<QVector<double>> &absEout, const QVector<QVector<double>> &normEout);
    void clearData();

private:
    QCustomPlot *customPlot;
};

#endif // PORTRAITWINDOW_H
