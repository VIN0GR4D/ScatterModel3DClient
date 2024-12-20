#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include <QDialog>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLogValueAxis>
#include <QPushButton>
#include <QCheckBox>

class GraphWindow : public QDialog {
    Q_OBJECT

public:
    explicit GraphWindow(QWidget *parent = nullptr);
    void setData(const QVector<double>& x, const QVector<double>& absY, const QVector<double>& normY);

private slots:
    void toggleAbsEout(bool checked);
    void toggleNormEout(bool checked);
    void resetZoom();
    void toggleLogarithmicScale(bool checked);
    void saveGraphAsPNG();
    void updateAxisRanges();

private:
    void setupUI();
    void createChart();
    void setupControls();

    QChart* chart;
    QChartView* chartView;
    QLineSeries* absEoutSeries;
    QLineSeries* normEoutSeries;
    QCheckBox* absEoutCheckBox;
    QCheckBox* normEoutCheckBox;
    QCheckBox* logScaleCheckBox;
    QPushButton* resetZoomButton;
    QPushButton* saveButton;

    QValueAxis* axisX;
    QValueAxis* axisY;
    QLogValueAxis* logAxisY;

    QRectF initialPlotArea;
    double minX, maxX, minY, maxY;
};

#endif // GRAPHWINDOW_H
