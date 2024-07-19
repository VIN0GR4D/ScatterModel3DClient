#include "portraitwindow.h"
#include <QDebug>
#include <QPainter>
#include <QPen>

PortraitWindow::PortraitWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("2D Portrait");
    resize(800, 600);
}

void PortraitWindow::setData(const QVector<double> &absEout, const QVector<double> &normEout)
{
    this->absEout = absEout;
    this->normEout = normEout;

    // Отладочный вывод
    qDebug() << "absEout:" << absEout;
    qDebug() << "normEout:" << normEout;

    update(); // Перерисовать окно
}

void PortraitWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    drawPortrait(painter);
}

void PortraitWindow::drawPortrait(QPainter &painter)
{
    if (absEout.isEmpty() || normEout.isEmpty())
        return;

    // Настройки рисования
    painter.setRenderHint(QPainter::Antialiasing);
    int width = this->width();
    int height = this->height();
    double maxAbsEout = *std::max_element(absEout.begin(), absEout.end());
    double maxNormEout = *std::max_element(normEout.begin(), normEout.end());

    // Отладочный вывод
    qDebug() << "Width:" << width << "Height:" << height;
    qDebug() << "Max absEout:" << maxAbsEout << "Max normEout:" << maxNormEout;

    // Рисуем оси
    painter.setPen(Qt::black);
    painter.drawLine(0, height / 2, width, height / 2); // Горизонтальная ось
    painter.drawLine(width / 2, 0, width / 2, height); // Вертикальная ось

    // Рисуем портрет по данным absEout
    painter.setPen(Qt::blue);
    QPen pointPen(Qt::blue);
    pointPen.setWidth(5);
    painter.setPen(pointPen);

    for (int i = 0; i < absEout.size(); ++i)
    {
        int x = (i * width) / absEout.size();
        int y = height - static_cast<int>((absEout[i] / maxAbsEout) * height);
        qDebug() << "Drawing point for absEout at:" << x << y;
        painter.drawPoint(x, y);

        // Добавляем линии между точками
        if (i > 0)
        {
            int prevX = ((i - 1) * width) / absEout.size();
            int prevY = height - static_cast<int>((absEout[i - 1] / maxAbsEout) * height);
            painter.drawLine(prevX, prevY, x, y);
        }
    }

    // Рисуем портрет по данным normEout
    painter.setPen(Qt::red);
    pointPen.setColor(Qt::red);
    painter.setPen(pointPen);

    for (int i = 0; i < normEout.size(); ++i)
    {
        int x = (i * width) / normEout.size();
        int y = height - static_cast<int>((normEout[i] / maxNormEout) * height);
        qDebug() << "Drawing point for normEout at:" << x << y;
        painter.drawPoint(x, y);

        // Добавляем линии между точками
        if (i > 0)
        {
            int prevX = ((i - 1) * width) / normEout.size();
            int prevY = height - static_cast<int>((normEout[i - 1] / maxNormEout) * height);
            painter.drawLine(prevX, prevY, x, y);
        }
    }
}
