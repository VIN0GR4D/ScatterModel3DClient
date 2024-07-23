#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include <QDialog>
#include "qcustomplot.h"

class GraphWindow : public QDialog {
    Q_OBJECT

public:
    explicit GraphWindow(QWidget *parent = nullptr);
    void setData(const QVector<double> &x, const QVector<double> &y1, const QVector<double> &y2);

private slots:
    void toggleAbsEout(bool checked);
    void toggleNormEout(bool checked);
    void resetZoom();

private:
    QCustomPlot *customPlot;
    QCheckBox *absEoutCheckBox, *normEoutCheckBox;
    QCPRange xRange, yRange;
};

#endif // GRAPHWINDOW_H
