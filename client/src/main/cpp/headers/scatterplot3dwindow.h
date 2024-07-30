#ifndef SCATTERPLOT3DWINDOW_H
#define SCATTERPLOT3DWINDOW_H

#include <QMainWindow>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QVector3D>
#include <QMatrix4x4>

class ScatterPlot3DWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit ScatterPlot3DWidget(QWidget *parent = nullptr);
    void setData(const QVector<QVector3D> &vertices, const QVector<QVector<int>> &indices);
    void setScatteringData(const QVector<double> &absEout, const QVector<double> &normEout);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;

private:
    void drawScatterPlot();
    void drawScattering();

    QVector<QVector3D> vertices;
    QVector<QVector<int>> indices;
    QVector<double> absEout;
    QVector<double> normEout;
    QMatrix4x4 projection;
    float rotationX, rotationY, rotationZ, scale;
    QPoint lastMousePosition;
    bool scatteringDataSet = false;
};

class ScatterPlot3DWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit ScatterPlot3DWindow(QWidget *parent = nullptr);
    void setData(const QVector<QVector3D> &vertices, const QVector<QVector<int>> &indices);
    void setScatteringData(const QVector<double> &absEout, const QVector<double> &normEout);

private:
    ScatterPlot3DWidget *openGLWidget;
};

#endif // SCATTERPLOT3DWINDOW_H
