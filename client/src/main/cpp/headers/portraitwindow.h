#ifndef PORTRAITWINDOW_H
#define PORTRAITWINDOW_H

#include <QDialog>
#include <QVector>
#include <QPainter>
#include <QColor>
#include <QMouseEvent>
#include <QWheelEvent>

class PortraitWindow : public QDialog {
    Q_OBJECT

public:
    explicit PortraitWindow(QWidget *parent = nullptr);
    void setData(const QVector<QVector<double>> &data);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    QVector<QVector<double>> data;
    QColor getColorForValue(double value) const;
    QPointF offset;
    double scale = 1.0;
    QPoint lastMousePos;
};

#endif // PORTRAITWINDOW_H
