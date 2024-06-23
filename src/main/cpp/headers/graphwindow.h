#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include <QDialog>
#include "qcustomplot.h"

class GraphWindow : public QDialog {
    Q_OBJECT

public:
    explicit GraphWindow(QWidget *parent = nullptr);
    void setData(const QVector<double> &x, const QVector<double> &y);

private:
    QCustomPlot *customPlot;
};

#endif // GRAPHWINDOW_H
