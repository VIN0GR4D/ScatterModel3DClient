#include "portraitwindow.h"
#include <QPainter>
#include <QPen>
#include <QDebug>
#include <QToolTip>

PortraitWindow::PortraitWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Двумерный портрет");
    resize(800, 600);
}

void PortraitWindow::setData(const QVector<double> &absEout, const QVector<double> &normEout)
{
    this->absEout = absEout;
    this->normEout = normEout;
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

    painter.setRenderHint(QPainter::Antialiasing);

    int width = this->width();
    int height = this->height();

    double maxAbsEout = *std::max_element(absEout.begin(), absEout.end());
    double maxNormEout = *std::max_element(normEout.begin(), normEout.end());

    // Рисуем сетку и оси
    drawGrid(painter, width, height);

    // Заголовок
    QFont titleFont = painter.font();
    titleFont.setPointSize(16);
    painter.setFont(titleFont);
    painter.drawText(width / 2 - 100, 30, "Двумерный портрет");

    // Легенда
    QFont legendFont = painter.font();
    legendFont.setPointSize(12);
    painter.setFont(legendFont);
    painter.setPen(Qt::blue);
    painter.drawText(10, height - 20, "Абсолютная величина");

    painter.setPen(Qt::red);
    painter.drawText(10, height - 40, "Нормированная величина");

    // Настройки для absEout
    QPen penAbsEout(Qt::blue, 2);
    painter.setPen(penAbsEout);

    QPolygon absEoutPolygon;
    for (int i = 0; i < absEout.size(); ++i)
    {
        int x = (i * width) / absEout.size();
        int y = height - static_cast<int>((absEout[i] / maxAbsEout) * height);
        absEoutPolygon << QPoint(x, y);
    }
    painter.drawPolyline(absEoutPolygon);

    // Настройки для normEout
    QPen penNormEout(Qt::red, 2);
    painter.setPen(penNormEout);

    QPolygon normEoutPolygon;
    for (int i = 0; i < normEout.size(); ++i)
    {
        int x = (i * width) / normEout.size();
        int y = height - static_cast<int>((normEout[i] / maxNormEout) * height);
        normEoutPolygon << QPoint(x, y);
    }
    painter.drawPolyline(normEoutPolygon);
}

void PortraitWindow::drawGrid(QPainter &painter, int width, int height)
{
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
    int numLines = 10;
    for (int i = 1; i < numLines; ++i)
    {
        int x = (i * width) / numLines;
        int y = (i * height) / numLines;
        painter.drawLine(x, 0, x, height);
        painter.drawLine(0, y, width, y);
    }

    painter.setPen(QPen(Qt::black, 2));
    painter.drawLine(0, height / 2, width, height / 2); // Горизонтальная ось
    painter.drawLine(width / 2, 0, width / 2, height); // Вертикальная ось

    // Подписи осей
    QFont font = painter.font();
    font.setPointSize(12);
    painter.setFont(font);
    painter.drawText(width - 20, height / 2 - 10, "X");
    painter.drawText(width / 2 + 10, 20, "Y");
}
