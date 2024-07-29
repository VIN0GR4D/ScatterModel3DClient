#ifndef PORTRAITWINDOW_H
#define PORTRAITWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QCheckBox>

class PortraitWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit PortraitWindow(QWidget *parent = nullptr);
    void setData(const QVector<double> &absEout, const QVector<double> &normEout);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void drawPortrait(QPainter &painter);
    void drawGrid(QPainter &painter, int width, int height);
    void drawData(QPainter &painter, const QVector<double> &data, QColor color, int width, int height, bool logScale);
    void drawTitle(QPainter &painter, int width);
    void drawLegend(QPainter &painter, int height);

    QVector<double> absEout;
    QVector<double> normEout;
    QCheckBox *logScaleCheckBox;
    bool logScale;

    QPoint lastMousePosition;
    double zoomFactor;
    QPointF panOffset;

private slots:
    void toggleLogarithmicScale(bool checked);
};

#endif // PORTRAITWINDOW_H
