#ifndef PORTRAITWINDOW_H
#define PORTRAITWINDOW_H

#include <QMainWindow>
#include <QVector>
#include <QPainter>

class PortraitWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit PortraitWindow(QWidget *parent = nullptr);
    void setData(const QVector<double> &absEout, const QVector<double> &normEout);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QVector<double> absEout;
    QVector<double> normEout;

    void drawPortrait(QPainter &painter);
};

#endif // PORTRAITWINDOW_H
