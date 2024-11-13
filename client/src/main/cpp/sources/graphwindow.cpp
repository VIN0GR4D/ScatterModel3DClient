#include "graphwindow.h"
#include <QVBoxLayout>
#include <QCheckBox>

GraphWindow::GraphWindow(QWidget *parent) : QDialog(parent) {
    customPlot = new QCustomPlot(this);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(customPlot);

    // Оси
    customPlot->xAxis->setLabel("Index");
    customPlot->yAxis->setLabel("Value");

    // Добавляем графики для absEout и normEout
    customPlot->addGraph(); // График для absEout
    customPlot->addGraph(); // График для normEout

    // Устанавливаем цвета для графиков
    QPen absEoutPen(Qt::blue); // Синий цвет для absEout
    QPen normEoutPen(Qt::red); // Красный цвет для normEout
    customPlot->graph(0)->setPen(absEoutPen);
    customPlot->graph(1)->setPen(normEoutPen);

    // Включаем масштабирование колесиком мыши
    customPlot->setInteraction(QCP::iRangeZoom, true);
    customPlot->setInteraction(QCP::iRangeDrag, true);
    customPlot->axisRect()->setRangeZoom(Qt::Horizontal | Qt::Vertical);  // Масштабирование по обеим осям
    customPlot->axisRect()->setRangeDrag(Qt::Horizontal | Qt::Vertical);  // Перемещение по обеим осям

    absEoutCheckBox = new QCheckBox("Показать absEout", this);
    absEoutCheckBox->setChecked(true);
    connect(absEoutCheckBox, &QCheckBox::toggled, this, &GraphWindow::toggleAbsEout);
    layout->addWidget(absEoutCheckBox);

    normEoutCheckBox = new QCheckBox("Показать normEout", this);
    normEoutCheckBox->setChecked(true);
    connect(normEoutCheckBox, &QCheckBox::toggled, this, &GraphWindow::toggleNormEout);
    layout->addWidget(normEoutCheckBox);

    logScaleCheckBox = new QCheckBox("Логарифмическая шкала", this);
    connect(logScaleCheckBox, &QCheckBox::toggled, this, &GraphWindow::toggleLogarithmicScale);
    layout->addWidget(logScaleCheckBox);

    // Добавляем кнопку сброса масштаба
    resetZoomButton = new QPushButton("Сбросить до исходного положения", this);
    connect(resetZoomButton, &QPushButton::clicked, this, &GraphWindow::resetZoom);
    layout->addWidget(resetZoomButton);

    // Создаем и добавляем кнопку сохранения графика
    saveButton = new QPushButton("Сохранить график как PNG", this);
    connect(saveButton, &QPushButton::clicked, this, &GraphWindow::saveGraphAsPNG);
    layout->addWidget(saveButton);

    setWindowTitle("Одномерный график");

    // Задаем размер окна
    this->resize(800, 600);
}

void GraphWindow::setData(const QVector<double> &x, const QVector<double> &absY, const QVector<double> &normY) {
    customPlot->graph(0)->setData(x, absY);
    customPlot->graph(1)->setData(x, normY);

    customPlot->rescaleAxes();

    // Сохраняем начальные диапазоны осей после вызова rescaleAxes
    xRange = customPlot->xAxis->range();
    yRange = customPlot->yAxis->range();

    customPlot->replot();

    // Для отладки
    qDebug() << "Graph data X:" << x;
    qDebug() << "Graph data absY:" << absY;
    qDebug() << "Graph data normY:" << normY;
}

void GraphWindow::toggleAbsEout(bool checked) {
    customPlot->graph(0)->setVisible(checked);
    customPlot->replot();
}

void GraphWindow::toggleNormEout(bool checked) {
    customPlot->graph(1)->setVisible(checked);
    customPlot->replot();
}

void GraphWindow::toggleLogarithmicScale(bool checked) {
    if (checked) {
        customPlot->yAxis->setScaleType(QCPAxis::stLogarithmic);
    } else {
        customPlot->yAxis->setScaleType(QCPAxis::stLinear);
    }
    customPlot->rescaleAxes();
    customPlot->replot();
}

void GraphWindow::resetZoom() {
    customPlot->xAxis->setRange(xRange);
    customPlot->yAxis->setRange(yRange);
    customPlot->replot();
}

void GraphWindow::saveGraphAsPNG() {
    // Открываем диалоговое окно для выбора места сохранения файла
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить график", "", "PNG Files (*.png)");
    if (fileName.isEmpty())
        return;

    // Захватываем текущий график как QPixmap
    QPixmap pixmap = customPlot->toPixmap(customPlot->width(), customPlot->height());

    // Определяем текст подписи в зависимости от типа шкалы
    QString scaleType = logScaleCheckBox->isChecked() ? "Логарифмическая шкала" : "Линейная шкала";

    // Создаем QPainter для рисования на pixmap
    QPainter painter(&pixmap);
    painter.setPen(Qt::black);
    QFont font = painter.font();
    font.setPointSize(12);
    painter.setFont(font);

    // Определяем позицию для подписи
    int margin = 10;
    painter.drawText(margin, margin + font.pointSize(), scaleType);

    painter.end(); // Завершаем рисование

    // Сохраняем pixmap в файл
    if (!pixmap.save(fileName, "PNG")) {
        QMessageBox::warning(this, "Ошибка", "Не удалось сохранить файл.");
    } else {
        QMessageBox::information(this, "Успех", "График успешно сохранен.");
    }
}
