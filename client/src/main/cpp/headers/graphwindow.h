#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include <QDialog>
#include "qcustomplot.h"
#include <QPushButton>

class GraphWindow : public QDialog {
    Q_OBJECT

public:
    explicit GraphWindow(QWidget *parent = nullptr);
    void setData(const QVector<double> &x, const QVector<double> &y1, const QVector<double> &y2);

private slots:
    void toggleAbsEout(bool checked);
    void toggleNormEout(bool checked);
    void resetZoom();
    void toggleLogarithmicScale(bool checked);
    void saveGraphAsPNG();

private:
    QCustomPlot *customPlot;
    QCheckBox *absEoutCheckBox, *normEoutCheckBox, *logScaleCheckBox;
    QPushButton *resetZoomButton, *saveButton;
    QCPRange xRange, yRange;
};

#endif // GRAPHWINDOW_H
