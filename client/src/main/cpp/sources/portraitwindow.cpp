#include "portraitwindow.h"
#include "qevent.h"
#include <QPainter>
#include <QPen>
#include <QDebug>
#include <QToolTip>
#include <QToolBar>
#include <cmath>

PortraitWindow::PortraitWindow(QWidget *parent)
    : QMainWindow(parent), logScale(false), zoomFactor(1.0), panOffset(0, 0) {
    setWindowTitle("Двумерный портрет");
    resize(800, 600);

    logScaleCheckBox = new QCheckBox("Логарифмическая шкала", this);
    connect(logScaleCheckBox, &QCheckBox::toggled, this, &PortraitWindow::toggleLogarithmicScale);
    QToolBar *toolBar = addToolBar("Toolbar");
    toolBar->addWidget(logScaleCheckBox);
}

void PortraitWindow::setData(const QVector<double> &absEout, const QVector<double> &normEout) {
    this->absEout = absEout;
    this->normEout = normEout;
    update(); // Перерисовать окно
}

void PortraitWindow::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    drawPortrait(painter);
}

void PortraitWindow::drawPortrait(QPainter &painter) {
    if (absEout.isEmpty() || normEout.isEmpty())
        return;

    painter.setRenderHint(QPainter::Antialiasing);

    int width = this->width();
    int height = this->height();

    drawGrid(painter, width, height);
    drawTitle(painter, width);
    drawLegend(painter, height);
    drawData(painter, absEout, Qt::blue, width, height, logScale);
    drawData(painter, normEout, Qt::red, width, height, logScale);
}

void PortraitWindow::drawGrid(QPainter &painter, int width, int height) {
    painter.setPen(QPen(Qt::lightGray, 1, Qt::DashLine));
    int numLines = 10;
    for (int i = 1; i < numLines; ++i) {
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

void PortraitWindow::drawData(QPainter &painter, const QVector<double> &data, QColor color, int width, int height, bool logScale) {
    QPen pen(color, 2);
    painter.setPen(pen);
    QPolygon polygon;
    double maxData = *std::max_element(data.begin(), data.end());
    for (int i = 0; i < data.size(); ++i) {
        int x = (i * width) / data.size();
        double value = logScale ? (data[i] > 0 ? std::log10(data[i]) : 0) : data[i];
        double maxValue = logScale ? (maxData > 0 ? std::log10(maxData) : 0) : maxData;
        int y = height - static_cast<int>((value / maxValue) * height);

        x = (x - width / 2) * zoomFactor + width / 2 + panOffset.x();
        y = (y - height / 2) * zoomFactor + height / 2 + panOffset.y();

        polygon << QPoint(x, y);
    }
    painter.drawPolyline(polygon);
}

void PortraitWindow::drawTitle(QPainter &painter, int width) {
    QFont titleFont = painter.font();
    titleFont.setPointSize(16);
    painter.setFont(titleFont);
    painter.drawText(width / 2 - 100, 30, "Двумерный портрет");
}

void PortraitWindow::drawLegend(QPainter &painter, int height) {
    QFont legendFont = painter.font();
    legendFont.setPointSize(12);
    painter.setFont(legendFont);
    painter.setPen(Qt::blue);
    painter.drawText(10, height - 20, "Абсолютная величина");
    painter.setPen(Qt::red);
    painter.drawText(10, height - 40, "Нормированная величина");
}

void PortraitWindow::toggleLogarithmicScale(bool checked) {
    logScale = checked;
    update(); // Перерисовать окно
}


void PortraitWindow::mousePressEvent(QMouseEvent *event) {
    lastMousePosition = event->pos();
}

void PortraitWindow::mouseMoveEvent(QMouseEvent *event) {
    QPoint delta = event->pos() - lastMousePosition;
    panOffset += delta / zoomFactor;
    lastMousePosition = event->pos();
    update();
}

void PortraitWindow::wheelEvent(QWheelEvent *event) {
    if (event->angleDelta().y() > 0) {
        zoomFactor *= 1.1;
    } else {
        zoomFactor /= 1.1;
    }
    update();
}
