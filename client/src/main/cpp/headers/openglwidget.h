#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QOpenGLFunctions_3_0>
#include <QSharedPointer>
#include <QMatrix4x4>
#include <cstddef>
#include "Triangle.h"
#include "raytracer.h"

class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_0 {
    Q_OBJECT

public:
    explicit OpenGLWidget(QWidget *parent = nullptr);
    void setGeometry(const QVector<QVector3D> &vertices, const QVector<QVector<int>> &triangleIndices, const QVector<QSharedPointer<triangle>> &triangles);
    QVector3D getCameraPosition() const;
    const QVector<QSharedPointer<triangle>>& getTriangles() const;
    void updateScene();
    void clearScene();
    rVect QVector3DToRVect(const QVector3D& vec) const;
    void setRotation(float x, float y, float z);
    const QVector<QVector3D>& getVertices() const;
    const QVector<QVector<int>>& getIndices() const;
    void setGridVisible(bool visible);

    void setObjectPosition(const QVector3D& position);
    QVector3D getObjectPosition() const;

    void setTriangles(const QVector<QSharedPointer<triangle>>& tri);
    void setScalingCoefficients(const QVector3D& scaling);
    void setShadowTrianglesFiltering(bool enable);

    bool hasShadowTriangles() const;
    void processShadowTriangles(const rVect& observerPosition);
    int getTotalTrianglesCount() const;
    int getVisibleTrianglesCount() const;

    // Метод для получения вектора направления, учитывающий текущие вращения
    rVect getDirectionVector() const;
    // Метод для получения позиции камеры в формате rVect
    rVect getCameraPositionAsRVect() const;

    // Метод для применения фильтрации треугольников
    void applyFilteredTriangles(const QVector<QSharedPointer<triangle>>& filteredTriangles);

    float getSurfaceAlpha() const { return surfaceAlpha; }
    void setSurfaceAlpha(float alpha) {
        surfaceAlpha = qBound(0.1f, alpha, 0.9f);  // Ограничиваем значения
        update();
    }

    // Метод для получения текущего значения rotation
    void getRotation(float &x, float &y, float &z) const;

    void resetScale();

public slots:
    void setUnderlyingSurfaceVisible(bool visible);

signals:
    void rotationChanged(float x, float y, float z);

private:
    RayTracer rayTracer;
    int currentWidth = 0;
    int currentHeight = 0;
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float rotationZ = 0.0f;
    QPoint lastMousePosition;
    float scale = 1.0f;
    QVector<QVector3D> vertices;
    QVector<QVector<int>> triangleIndices;
    QVector<QSharedPointer<triangle>> triangles;
    QVector<QColor> triangleColors;
    QVector<QColor> colorTable;
    QVector3D objectPosition;
    QVector3D cameraPosition;
    QVector3D cameraFront;
    QVector3D cameraUp;
    QVector3D lightPosition;
    QMatrix4x4 projection;

    QVector3D boundingSphereCenter;
    float boundingSphereRadius;
    bool geometryChanged = true;
    bool manualCameraControl = false;

    QColor chooseColorForTriangle(int triangleIndex);
    QVector3D computeBoundingSphereCenter(const QVector<QVector3D> &vertices);
    float computeBoundingSphereRadius(const QVector<QVector3D> &vertices, const QVector3D &center);
    float calculateOptimalZoomOutFactor(float boundingSphereRadius);
    void computeCameraPosition();
    void applyTransformations();
    void updateSceneBoundingVolume();
    void updateCameraPosition(const QVector3D& center, float radius);
    void computeBoundingVolume();
    void updateVisibility(const QVector<QSharedPointer<triangle>>& triangles);
    void updateVisibility();
    void resetCamera();

    QVector3D projectToScreen(const QVector3D& worldCoord, const QMatrix4x4& modelView, const QMatrix4x4& projection, int windowWidth, int windowHeight);

    void drawGrid();
    bool gridVisible;
    void drawCoordinateIndicator();
    void drawAxisWithArrow(float axisLength);

    // Размеры сетки и шаг
    float gridSize;
    float gridStep;

    // Цвета основных осей (X, Y, Z)
    QColor axisColors[3];

    // Минимальное и максимальное значение альфа для сетки
    const float minAlpha = 0.2f;
    const float maxAlpha = 1.0f;

    // Переменные для хранения прозрачности
    float gridAlpha;

    // Флаг для обновления сетки
    bool gridParametersChanged;

    bool showUnderlyingSurface;
    float surfaceAlpha = 0.3f;
    void drawUnderlyingSurface();

    bool filterShadowTriangles = false;  // Флаг фильтрации теневых треугольников

    void initializeDefaultGrid();
    bool hasLoadedObject;  // флаг наличия загруженного объекта

    float computeMinZ() const;

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
};

#endif // OPENGLWIDGET_H
