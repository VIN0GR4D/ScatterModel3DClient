#ifndef HEATMAPWINDOW_H
#define HEATMAPWINDOW_H

#include <QMainWindow>
#include <QVector>

class HeatmapWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit HeatmapWindow(QWidget *parent = nullptr);
    void setData(const QVector<QVector<double>> &data);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    void drawHeatmap(QPainter &painter);
    void drawLegend(QPainter &painter, double minVal, double maxVal);
    QVector<QVector<double>> data;
};

#endif // HEATMAPWINDOW_H
