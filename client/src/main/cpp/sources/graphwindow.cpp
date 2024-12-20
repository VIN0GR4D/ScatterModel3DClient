#include "graphwindow.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QFileDialog>
#include <QPainter>
#include <QtMath>
#include <algorithm>

GraphWindow::GraphWindow(QWidget *parent)
    : QDialog(parent)
    , chart(new QChart())
    , absEoutSeries(new QLineSeries())
    , normEoutSeries(new QLineSeries())
    , axisX(new QValueAxis())
    , axisY(new QValueAxis())
    , logAxisY(new QLogValueAxis())
{
    setupUI();
    createChart();
    setupControls();

    setWindowTitle(tr("Одномерный график"));
    resize(800, 600);
}

void GraphWindow::setupUI() {
    QVBoxLayout* layout = new QVBoxLayout(this);

    // Create chart view
    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    layout->addWidget(chartView);

    // Create control widgets
    absEoutCheckBox = new QCheckBox(tr("Показать absEout"), this);
    absEoutCheckBox->setChecked(true);
    layout->addWidget(absEoutCheckBox);

    normEoutCheckBox = new QCheckBox(tr("Показать normEout"), this);
    normEoutCheckBox->setChecked(true);
    layout->addWidget(normEoutCheckBox);

    logScaleCheckBox = new QCheckBox(tr("Логарифмическая шкала"), this);
    layout->addWidget(logScaleCheckBox);

    resetZoomButton = new QPushButton(tr("Сбросить до исходного положения"), this);
    layout->addWidget(resetZoomButton);

    saveButton = new QPushButton(tr("Сохранить график как PNG"), this);
    layout->addWidget(saveButton);

    // Connect signals
    connect(absEoutCheckBox, &QCheckBox::toggled, this, &GraphWindow::toggleAbsEout);
    connect(normEoutCheckBox, &QCheckBox::toggled, this, &GraphWindow::toggleNormEout);
    connect(logScaleCheckBox, &QCheckBox::toggled, this, &GraphWindow::toggleLogarithmicScale);
    connect(resetZoomButton, &QPushButton::clicked, this, &GraphWindow::resetZoom);
    connect(saveButton, &QPushButton::clicked, this, &GraphWindow::saveGraphAsPNG);
}

void GraphWindow::createChart() {
    // Setup series
    absEoutSeries->setName(tr("absEout"));
    absEoutSeries->setPen(QPen(Qt::blue, 2));
    normEoutSeries->setName(tr("normEout"));
    normEoutSeries->setPen(QPen(Qt::red, 2));

    chart->addSeries(absEoutSeries);
    chart->addSeries(normEoutSeries);

    // Setup axes
    axisX->setTitleText(tr("Index"));
    axisY->setTitleText(tr("Value"));
    logAxisY->setTitleText(tr("Value"));
    logAxisY->setBase(10.0);
    logAxisY->setMinorTickCount(-1);

    // Add axes to chart
    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    absEoutSeries->attachAxis(axisX);
    absEoutSeries->attachAxis(axisY);
    normEoutSeries->attachAxis(axisX);
    normEoutSeries->attachAxis(axisY);

    // Enable zooming and panning
    chartView->setRubberBand(QChartView::RectangleRubberBand);
}

void GraphWindow::setupControls() {
    chartView->setInteractive(true);
    chartView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    chartView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
}

void GraphWindow::setData(const QVector<double>& x, const QVector<double>& absY, const QVector<double>& normY) {
    // Clear existing data
    absEoutSeries->clear();
    normEoutSeries->clear();

    // Find data ranges
    minX = x.isEmpty() ? 0 : *std::min_element(x.begin(), x.end());
    maxX = x.isEmpty() ? 1 : *std::max_element(x.begin(), x.end());

    auto minMaxY = std::minmax_element(absY.begin(), absY.end());
    minY = *minMaxY.first;
    maxY = *minMaxY.second;

    auto minMaxNormY = std::minmax_element(normY.begin(), normY.end());
    minY = qMin(minY, *minMaxNormY.first);
    maxY = qMax(maxY, *minMaxNormY.second);

    // Add data points
    for(int i = 0; i < x.size(); ++i) {
        absEoutSeries->append(x[i], absY[i]);
        normEoutSeries->append(x[i], normY[i]);
    }

    updateAxisRanges();
    initialPlotArea = chart->plotArea();

    // Debug output
    qDebug() << "Graph data X:" << x;
    qDebug() << "Graph data absY:" << absY;
    qDebug() << "Graph data normY:" << normY;
}

void GraphWindow::toggleAbsEout(bool checked) {
    absEoutSeries->setVisible(checked);
    updateAxisRanges();
}

void GraphWindow::toggleNormEout(bool checked) {
    normEoutSeries->setVisible(checked);
    updateAxisRanges();
}

void GraphWindow::updateAxisRanges() {
    // Calculate visible data ranges
    double visibleMinY = maxY;
    double visibleMaxY = minY;

    if (absEoutSeries->isVisible()) {
        const auto& points = absEoutSeries->points();
        for (const auto& point : points) {
            visibleMinY = qMin(visibleMinY, point.y());
            visibleMaxY = qMax(visibleMaxY, point.y());
        }
    }

    if (normEoutSeries->isVisible()) {
        const auto& points = normEoutSeries->points();
        for (const auto& point : points) {
            visibleMinY = qMin(visibleMinY, point.y());
            visibleMaxY = qMax(visibleMaxY, point.y());
        }
    }

    // Add margins
    double xMargin = (maxX - minX) * 0.05;
    double yMargin = (visibleMaxY - visibleMinY) * 0.05;

    axisX->setRange(minX - xMargin, maxX + xMargin);

    if (logScaleCheckBox->isChecked()) {
        logAxisY->setRange(qMax(visibleMinY, 0.00001), visibleMaxY * 1.1);
    } else {
        axisY->setRange(visibleMinY - yMargin, visibleMaxY + yMargin);
    }
}

void GraphWindow::toggleLogarithmicScale(bool logarithmic) {
    QAbstractAxis* currentYAxis = chart->axes(Qt::Vertical).first();
    chart->removeAxis(currentYAxis);

    if (logarithmic) {
        chart->addAxis(logAxisY, Qt::AlignLeft);
        absEoutSeries->attachAxis(logAxisY);
        normEoutSeries->attachAxis(logAxisY);
    } else {
        chart->addAxis(axisY, Qt::AlignLeft);
        absEoutSeries->attachAxis(axisY);
        normEoutSeries->attachAxis(axisY);
    }

    updateAxisRanges();
}

void GraphWindow::resetZoom() {
    chart->zoomReset();
    updateAxisRanges();
}

void GraphWindow::saveGraphAsPNG() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить график"), "", tr("PNG Files (*.png)"));
    if (fileName.isEmpty()) {
        return;
    }

    QPixmap pixmap(chartView->size());
    pixmap.fill(Qt::white);
    QPainter painter(&pixmap);

    // Render chart
    chartView->render(&painter);

    // Add scale type text
    painter.setPen(Qt::black);
    QFont font = painter.font();
    font.setPointSize(12);
    painter.setFont(font);
    QString scaleType = logScaleCheckBox->isChecked() ? tr("Логарифмическая шкала") : tr("Линейная шкала");
    painter.drawText(10, font.pointSize() + 10, scaleType);

    painter.end();

    if (!pixmap.save(fileName, "PNG")) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось сохранить файл."));
    } else {
        QMessageBox::information(this, tr("Успех"), tr("График успешно сохранен."));
    }
}
