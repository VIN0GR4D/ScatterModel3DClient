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
    , sliceControlWidget(nullptr)
    , sliceSpinBox(nullptr)
    , sliceLabel(nullptr)
    , sliceInfoLabel(nullptr)
    , freqBand(5) // Значение по умолчанию - Ka-диапазон
    , portraitType(PortraitDimension::Undefined)
    , isAngleSelected(false)
    , isAzimuthSelected(false)
    , isRangeSelected(false)
    , isMultidimensional(false)
    , currentSliceIndex(0)
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

    // Добавляем метку для дополнительной информации о срезе
    sliceInfoLabel = new QLabel(this);
    sliceInfoLabel->setAlignment(Qt::AlignCenter);
    sliceInfoLabel->setVisible(false);
    layout->addWidget(sliceInfoLabel);

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

    // Создаем контейнер для элементов управления срезом (будет настраиваться позже)
    sliceControlWidget = new QWidget(this);
    sliceControlWidget->setVisible(false);
    layout->addWidget(sliceControlWidget);

    // Connect signals
    connect(absEoutCheckBox, &QCheckBox::toggled, this, &GraphWindow::toggleAbsEout);
    connect(normEoutCheckBox, &QCheckBox::toggled, this, &GraphWindow::toggleNormEout);
    connect(logScaleCheckBox, &QCheckBox::toggled, this, &GraphWindow::toggleLogarithmicScale);
    connect(resetZoomButton, &QPushButton::clicked, this, &GraphWindow::resetZoom);
    connect(saveButton, &QPushButton::clicked, this, &GraphWindow::saveGraphAsPNG);
}

void GraphWindow::setupSliceControls() {
    if (!isMultidimensional) {
        sliceControlWidget->setVisible(false);
        return;
    }

    // Очищаем существующую разметку, если есть
    if (sliceControlWidget->layout()) {
        QLayoutItem* child;
        while ((child = sliceControlWidget->layout()->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }
        delete sliceControlWidget->layout();
    }

    QHBoxLayout* sliceLayout = new QHBoxLayout(sliceControlWidget);

    sliceLabel = new QLabel(tr("Выбрать срез:"), sliceControlWidget);
    sliceSpinBox = new QSpinBox(sliceControlWidget);

    // Настройка спинбокса в зависимости от размерности данных
    int maxSlice = 0;
    if (!absEoutData2D.isEmpty()) {
        maxSlice = absEoutData2D.size() - 1;
    }

    sliceSpinBox->setMinimum(0);
    sliceSpinBox->setMaximum(maxSlice);
    sliceSpinBox->setValue(currentSliceIndex);

    sliceLayout->addWidget(sliceLabel);
    sliceLayout->addWidget(sliceSpinBox);

    sliceControlWidget->setVisible(true);

    connect(sliceSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            this, &GraphWindow::sliceChanged);
}

void GraphWindow::updateSliceUI() {
    if (!isMultidimensional || absEoutData2D.isEmpty()) {
        sliceInfoLabel->setVisible(false);
        return;
    }

    QString sliceInfo;
    switch (portraitType) {
    case PortraitDimension::AzimuthRange:
        sliceInfo = tr("Азимутальный срез %1 из %2").arg(currentSliceIndex + 1).arg(absEoutData2D.size());
        break;
    case PortraitDimension::ElevationRange:
        sliceInfo = tr("Угломестный срез %1 из %2").arg(currentSliceIndex + 1).arg(absEoutData2D.size());
        break;
    case PortraitDimension::ElevationAzimuth:
        sliceInfo = tr("Угломестный срез %1 из %2").arg(currentSliceIndex + 1).arg(absEoutData2D.size());
        break;
    case PortraitDimension::ThreeDimensional:
        if (isAngleSelected && isAzimuthSelected) {
            sliceInfo = tr("Срез по дальности %1 из %2").arg(currentSliceIndex + 1).arg(absEoutData2D.size());
        } else if (isAngleSelected && isRangeSelected) {
            sliceInfo = tr("Срез по азимуту %1 из %2").arg(currentSliceIndex + 1).arg(absEoutData2D.size());
        } else if (isAzimuthSelected && isRangeSelected) {
            sliceInfo = tr("Срез по углу места %1 из %2").arg(currentSliceIndex + 1).arg(absEoutData2D.size());
        }
        break;
    default:
        sliceInfo = tr("Срез %1 из %2").arg(currentSliceIndex + 1).arg(absEoutData2D.size());
        break;
    }

    sliceInfoLabel->setText(sliceInfo);
    sliceInfoLabel->setVisible(true);
}

void GraphWindow::createChart() {
    // Setup series
    absEoutSeries->setName(tr("Абсолютное значение (absEout)"));
    absEoutSeries->setPen(QPen(Qt::blue, 2));
    normEoutSeries->setName(tr("Нормированное значение (normEout)"));
    normEoutSeries->setPen(QPen(Qt::red, 2));

    chart->addSeries(absEoutSeries);
    chart->addSeries(normEoutSeries);

    // Set default axis titles
    xAxisTitle = tr("Индекс");
    yAxisTitle = tr("Интенсивность рассеяния, дБ");

    // Setup axes
    axisX->setTitleText(xAxisTitle);
    axisY->setTitleText(yAxisTitle);
    logAxisY->setTitleText(yAxisTitle);
    logAxisY->setBase(10.0);
    logAxisY->setMinorTickCount(-1);

    // Включаем отображение легенды
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);

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
    // Сохраняем данные
    xData = x;
    absEoutData = absY;
    normEoutData = normY;
    isMultidimensional = false;

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

    // Обновляем заголовки осей с учетом типа портрета
    updateAxisTitles();

    // Обновляем диапазоны осей
    updateAxisRanges();
    initialPlotArea = chart->plotArea();

    // Скрываем элементы управления для многомерных данных
    sliceControlWidget->setVisible(false);
    sliceInfoLabel->setVisible(false);

    // Debug output
    qDebug() << "Graph data X:" << x;
    qDebug() << "Graph data absY:" << absY;
    qDebug() << "Graph data normY:" << normY;
}

void GraphWindow::setData2D(const QVector<QVector<double>>& absData2D, const QVector<QVector<double>>& normData2D) {
    if (absData2D.isEmpty() || normData2D.isEmpty()) {
        return;
    }

    // Сохраняем двумерные данные
    absEoutData2D = absData2D;
    normEoutData2D = normData2D;
    isMultidimensional = true;
    currentSliceIndex = 0;

    // Создаем одномерные данные из первого среза
    QVector<double> x;
    for (int i = 0; i < absData2D[0].size(); ++i) {
        x.append(i);
    }

    // Установить одномерные данные из первого среза
    setData(x, absData2D[0], normData2D[0]);

    // Настраиваем элементы управления для многомерных данных
    setupSliceControls();
    updateSliceUI();
}

void GraphWindow::sliceChanged(int index) {
    if (!isMultidimensional || absEoutData2D.isEmpty() || index >= absEoutData2D.size()) {
        return;
    }

    currentSliceIndex = index;

    // Очищаем серии
    absEoutSeries->clear();
    normEoutSeries->clear();

    // Создаем данные оси X
    QVector<double> x;
    for (int i = 0; i < absEoutData2D[index].size(); ++i) {
        x.append(i);
    }

    // Обновляем минимум и максимум
    minX = 0;
    maxX = absEoutData2D[index].size() - 1;

    auto minMaxY = std::minmax_element(absEoutData2D[index].begin(), absEoutData2D[index].end());
    minY = *minMaxY.first;
    maxY = *minMaxY.second;

    auto minMaxNormY = std::minmax_element(normEoutData2D[index].begin(), normEoutData2D[index].end());
    minY = qMin(minY, *minMaxNormY.first);
    maxY = qMax(maxY, *minMaxNormY.second);

    // Добавляем данные в серии
    for (int i = 0; i < x.size(); ++i) {
        absEoutSeries->append(x[i], absEoutData2D[index][i]);
        normEoutSeries->append(x[i], normEoutData2D[index][i]);
    }

    // Обновляем интерфейс и диапазоны осей
    updateSliceUI();
    updateAxisRanges();
}

void GraphWindow::setFreqBand(int band) {
    freqBand = band;
    updateAxisTitles();
}

void GraphWindow::setPortraitType(PortraitDimension type) {
    portraitType = type;

    // Определяем многомерность данных
    isMultidimensional = (type >= PortraitDimension::AzimuthRange);

    updateAxisTitles();
}

void GraphWindow::setPortraitSelections(bool angle, bool azimuth, bool range) {
    isAngleSelected = angle;
    isAzimuthSelected = azimuth;
    isRangeSelected = range;

    updateAxisTitles();
    updateSliceUI();
}

void GraphWindow::setInfoText(const QString& text) {
    chart->setTitle(text);
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

void GraphWindow::updateAxisTitles() {
    // Определяем названия осей в зависимости от типа портрета
    switch (portraitType) {
    case PortraitDimension::Range:
        xAxisTitle = tr("Расстояние, м");
        break;
    case PortraitDimension::Azimuth:
        xAxisTitle = tr("Азимут, град.");
        break;
    case PortraitDimension::Elevation:
        xAxisTitle = tr("Угол места, град.");
        break;
    case PortraitDimension::AzimuthRange:
        if (isAzimuthSelected && isRangeSelected) {
            xAxisTitle = tr("Индекс (азимутально-дальностная плоскость)");
        } else if (isAzimuthSelected) {
            xAxisTitle = tr("Азимут, град.");
        } else if (isRangeSelected) {
            xAxisTitle = tr("Расстояние, м");
        } else {
            xAxisTitle = tr("Индекс");
        }
        break;
    case PortraitDimension::ElevationRange:
        if (isAngleSelected && isRangeSelected) {
            xAxisTitle = tr("Индекс (угломестно-дальностная плоскость)");
        } else if (isAngleSelected) {
            xAxisTitle = tr("Угол места, град.");
        } else if (isRangeSelected) {
            xAxisTitle = tr("Расстояние, м");
        } else {
            xAxisTitle = tr("Индекс");
        }
        break;
    case PortraitDimension::ElevationAzimuth:
        if (isAngleSelected && isAzimuthSelected) {
            xAxisTitle = tr("Индекс (угломестно-азимутальная плоскость)");
        } else if (isAngleSelected) {
            xAxisTitle = tr("Угол места, град.");
        } else if (isAzimuthSelected) {
            xAxisTitle = tr("Азимут, град.");
        } else {
            xAxisTitle = tr("Индекс");
        }
        break;
    case PortraitDimension::ThreeDimensional:
        if (isAngleSelected && isAzimuthSelected && isRangeSelected) {
            xAxisTitle = tr("Индекс (трехмерный портрет)");
        } else if (isAngleSelected && isAzimuthSelected) {
            xAxisTitle = tr("Индекс (угломестно-азимутальная плоскость)");
        } else if (isAngleSelected && isRangeSelected) {
            xAxisTitle = tr("Индекс (угломестно-дальностная плоскость)");
        } else if (isAzimuthSelected && isRangeSelected) {
            xAxisTitle = tr("Индекс (азимутально-дальностная плоскость)");
        } else if (isAngleSelected) {
            xAxisTitle = tr("Угол места, град.");
        } else if (isAzimuthSelected) {
            xAxisTitle = tr("Азимут, град.");
        } else if (isRangeSelected) {
            xAxisTitle = tr("Расстояние, м");
        } else {
            xAxisTitle = tr("Индекс");
        }
        break;
    default:
        xAxisTitle = tr("Индекс");
        break;
    }

    // Определяем единицы измерения в зависимости от частотного диапазона для оси Y
    yAxisTitle = tr("Интенсивность рассеяния, дБ");

    // Обновляем заголовки осей на графике
    axisX->setTitleText(xAxisTitle);
    axisY->setTitleText(yAxisTitle);
    logAxisY->setTitleText(yAxisTitle);

    // Обновляем названия серий
    absEoutSeries->setName(tr("Абсолютное значение (absEout)"));
    normEoutSeries->setName(tr("Нормированное значение (normEout)"));
}

QString GraphWindow::getPortraitTypeName() const {
    switch (portraitType) {
    case PortraitDimension::Range:
        return tr("Дальностный портрет");
    case PortraitDimension::Azimuth:
        return tr("Азимутальный портрет");
    case PortraitDimension::Elevation:
        return tr("Угломестный портрет");
    case PortraitDimension::AzimuthRange:
        return tr("Азимутально-дальностный портрет");
    case PortraitDimension::ElevationRange:
        return tr("Угломестно-дальностный портрет");
    case PortraitDimension::ElevationAzimuth:
        return tr("Угломестно-азимутальный портрет");
    case PortraitDimension::ThreeDimensional:
        return tr("Трехмерный портрет");
    default:
        return tr("Неизвестный тип портрета");
    }
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

    // Add additional information
    painter.setPen(Qt::black);
    QFont font = painter.font();
    font.setPointSize(12);
    painter.setFont(font);

    // Тип шкалы (логарифмическая/линейная)
    QString scaleType = logScaleCheckBox->isChecked() ? tr("Логарифмическая шкала") : tr("Линейная шкала");

    // Название частотного диапазона
    QString freqBandName;
    switch (freqBand) {
    case 0: freqBandName = tr("P-диапазон (400-450 МГц)"); break;
    case 1: freqBandName = tr("L-диапазон (1-1.5 ГГц)"); break;
    case 2: freqBandName = tr("S-диапазон (2.75-3.15 ГГц)"); break;
    case 3: freqBandName = tr("C-диапазон (5-5.5 ГГц)"); break;
    case 4: freqBandName = tr("X-диапазон (9-10 ГГц)"); break;
    case 5: freqBandName = tr("Ka-диапазон (36.5-38.5 ГГц)"); break;
    default: freqBandName = tr("Неизвестный диапазон");
    }

    QString portraitTypeName = getPortraitTypeName();
    QString sliceInfo;

    if (isMultidimensional) {
        sliceInfo = sliceInfoLabel->text();
    }

    // Рисуем информацию на изображении
    int yPos = font.pointSize() + 10;
    painter.drawText(10, yPos, scaleType);
    yPos += font.pointSize() + 5;
    painter.drawText(10, yPos, freqBandName);
    yPos += font.pointSize() + 5;
    painter.drawText(10, yPos, portraitTypeName);
    yPos += font.pointSize() + 5;

    // Добавляем информацию о срезе, если это многомерные данные
    if (isMultidimensional && !sliceInfo.isEmpty()) {
        yPos += font.pointSize() + 5;
        painter.drawText(10, yPos, sliceInfo);
    }

    painter.end();

    if (!pixmap.save(fileName, "PNG")) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось сохранить файл."));
    } else {
        QMessageBox::information(this, tr("Успех"), tr("График успешно сохранен."));
    }
}
