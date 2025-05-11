#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include <QDialog>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QLogValueAxis>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QHBoxLayout>

// Включаем определение перечисления PortraitDimension
#include "portraittypes.h"

class GraphWindow : public QDialog {
    Q_OBJECT

public:
    explicit GraphWindow(QWidget *parent = nullptr);

    // Установка одномерных данных
    void setData(const QVector<double>& x, const QVector<double>& absY, const QVector<double>& normY);

    // Установка двумерных данных для поддержки многомерных портретов
    void setData2D(const QVector<QVector<double>>& absData2D,
                   const QVector<QVector<double>>& normData2D);

    // Установка параметров отображения
    void setFreqBand(int band);
    void setPortraitType(PortraitDimension type);
    void setPortraitSelections(bool angle, bool azimuth, bool range);
    void setInfoText(const QString& text);

private slots:
    void toggleAbsEout(bool checked);
    void toggleNormEout(bool checked);
    void resetZoom();
    void toggleLogarithmicScale(bool checked);
    void saveGraphAsPNG();
    void updateAxisRanges();
    void sliceChanged(int value); // Обработка изменения среза для многомерных данных

private:
    void setupUI();
    void createChart();
    void setupControls();
    void updateAxisTitles();   // Обновление заголовков осей
    void setupSliceControls(); // Настройка элементов управления для срезов
    void updateSliceUI();      // Обновление интерфейса выбора среза
    QString getPortraitTypeName() const; // Получение названия типа портрета

    QChart* chart;
    QChartView* chartView;
    QLineSeries* absEoutSeries;
    QLineSeries* normEoutSeries;
    QCheckBox* absEoutCheckBox;
    QCheckBox* normEoutCheckBox;
    QCheckBox* logScaleCheckBox;
    QPushButton* resetZoomButton;
    QPushButton* saveButton;

    QValueAxis* axisX;
    QValueAxis* axisY;
    QLogValueAxis* logAxisY;

    // Элементы управления для многомерных данных
    QWidget* sliceControlWidget;
    QSpinBox* sliceSpinBox;
    QLabel* sliceLabel;
    QLabel* sliceInfoLabel;

    // Данные и параметры
    QVector<double> xData;         // Данные оси X
    QVector<double> absEoutData;   // Одномерные абсолютные данные
    QVector<double> normEoutData;  // Одномерные нормированные данные
    QVector<QVector<double>> absEoutData2D;  // Двумерные абсолютные данные
    QVector<QVector<double>> normEoutData2D; // Двумерные нормированные данные

    QString xAxisTitle;            // Заголовок оси X
    QString yAxisTitle;            // Заголовок оси Y
    int freqBand;                  // Частотный диапазон
    PortraitDimension portraitType; // Тип портрета
    bool isAngleSelected;          // Выбран угломестный портрет
    bool isAzimuthSelected;        // Выбран азимутальный портрет
    bool isRangeSelected;          // Выбран дальностный портрет
    bool isMultidimensional;       // Признак многомерных данных

    QRectF initialPlotArea;
    double minX, maxX, minY, maxY;
    int currentSliceIndex;         // Текущий индекс среза для многомерных данных
};

#endif // GRAPHWINDOW_H
