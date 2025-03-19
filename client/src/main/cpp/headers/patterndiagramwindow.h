#ifndef PATTERNDIAGRAMWINDOW_H
#define PATTERNDIAGRAMWINDOW_H

#include <QDialog>
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QVector>
#include <QVector3D>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QLabel>
#include <QSlider>
#include <QGridLayout>
#include <QtMath>
#include <cmath>

class PatternDiagramWindow : public QDialog {
    Q_OBJECT

public:
    explicit PatternDiagramWindow(QWidget *parent = nullptr);
    void setData(const QVector<QVector<double>>& data);

    // Установка углов обзора
    void setAzimuthAngle(double angle);
    void setElevationAngle(double angle);

    int getProjectionType() const { return m_projectionType; }

    void drawPolarPattern(QPainter &painter, const QRect &rect);
    void draw3DPattern(QPainter &painter, const QRect &rect);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private slots:
    void toggleNormalization(bool normalized);
    void toggleLogScale(bool useLog);
    void toggleFillMode(bool fill);
    void saveImage();
    void resetView();
    void updateProjectionType(int index);
    void updateSliceAngle(int angle);

private:
    QVector<QVector<double>> m_data;
    QPoint m_lastMousePos;
    double m_scale;
    double m_azimuthAngle;
    double m_elevationAngle;

    // Параметры отображения
    bool m_normalized;
    bool m_logarithmicScale;
    bool m_fillPattern;
    int m_projectionType; // 0 - полярная, 1 - 3D
    int m_sliceAngle;     // Угол среза для 2D проекции

    // Максимальное и минимальное значения для нормализации
    double m_maxValue;
    double m_minValue;

    // UI элементы
    QPushButton *m_saveButton;
    QPushButton *m_resetButton;
    QComboBox *m_projectionCombo;
    QCheckBox *m_normalizeCheck;
    QCheckBox *m_logScaleCheck;
    QCheckBox *m_fillCheck;
    QSlider *m_sliceSlider;
    QLabel *m_sliceLabel;
    QVBoxLayout *m_controlsLayout;

    // Вспомогательные методы для рисования
    void setupUI();
    QColor getColorForValue(double value) const;
    double normalizeValue(double value) const;
    void calculateMinMax();

    // Вспомогательные методы для координат
    QPointF polarToCartesian(double radius, double angle) const;
    QPointF sphericalToCartesian(double radius, double theta, double phi) const;

    QWidget *m_drawingArea;
};

#endif // PATTERNDIAGRAMWINDOW_H
