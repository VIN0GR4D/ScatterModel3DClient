#ifndef PORTRAITWIDGET_H
#define PORTRAITWIDGET_H

#include <QWidget>
#include "qcustomplot.h"

class PortraitWidget : public QWidget {
    Q_OBJECT
public:
    explicit PortraitWidget(QWidget *parent = nullptr);
    void setData(const QVector<double> &x, const QVector<double> &y, const QVector<double> &z);
    void clearData();

private:
    QCustomPlot *customPlot;
    QCPColorMap *colorMap;
};

#endif // PORTRAITWIDGET_H
