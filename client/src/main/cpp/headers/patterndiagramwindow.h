#ifndef PATTERNDIAGRAMWINDOW_H
#define PATTERNDIAGRAMWINDOW_H

#include "qgroupbox.h"
#include <QDialog>
#include <QVector>
#include <QPainter>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPushButton>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QCheckBox>
#include <QSlider>
#include <QColorDialog>
#include <QComboBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>

class GLPatternWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    // Определяем перечисление для режимов отображения
    enum ViewMode {
        PointsMode = 0,
        SurfaceMode = 1,
        SliceMode = 2,
        SphereMode = 3
    };

    explicit GLPatternWidget(QWidget *parent = nullptr);
    void setData(const QVector<QVector<QVector<double>>> &data);
    void setThreshold(double value);
    void setColorMode(int mode);
    void setViewMode(ViewMode mode);
    void resetView();

    void setSliceIndex(int index);

protected:
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    void updateVertexBuffer();
    void generateSurfaceMesh();
    void updateSliceData();
    void generateColorMap();
    QColor getColorForValue(double value) const;

    QVector<QVector<QVector<double>>> data3D;
    QVector<GLfloat> vertices;
    QVector<GLfloat> colors;

    QVector<GLfloat> surfaceVertices;
    QVector<GLfloat> surfaceColors;

    QVector<GLfloat> sliceVertices;
    QVector<GLfloat> sliceColors;

    QMatrix4x4 projMatrix;
    QMatrix4x4 viewMatrix;
    QMatrix4x4 modelMatrix;

    QPoint lastMousePos;
    float xRot, yRot, zRot;
    float scale;
    float threshold;
    int colorMode;
    ViewMode viewMode;
    int sliceIndex;
    double maxValue;
    double minValue;

    void generateSphericalMesh();
    QVector<GLfloat> sphereVertices;
};

class PatternDiagramWindow : public QDialog {
    Q_OBJECT

public:
    explicit PatternDiagramWindow(QWidget *parent = nullptr);
    void setData(const QVector<QVector<QVector<double>>> &data);
    void setData(const QVector<QVector<double>> &data2D);

private slots:
    void savePatternAsPNG();
    void onThresholdChanged(int value);
    void onColorModeChanged(int index);
    void onViewModeChanged(int index);
    void resetView();

private:
    void setupUI();
    void convert2DTo3D(const QVector<QVector<double>> &data2D);

    GLPatternWidget *glWidget;
    QPushButton *saveButton;
    QPushButton *resetViewButton;
    QSlider *thresholdSlider;
    QComboBox *colorModeCombo;
    QComboBox *viewModeCombo;
    QVector<QVector<QVector<double>>> data3D;

    QSlider *sliceSlider;
    QLabel *sliceLabel;

    void onSliceChanged(int value);

    QGroupBox *sliceControlGroup;
};

#endif // PATTERNDIAGRAMWINDOW_H
