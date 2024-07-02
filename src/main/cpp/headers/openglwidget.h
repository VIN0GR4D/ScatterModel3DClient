#ifndef OPENGLWIDGET_H
#define OPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QOpenGLFunctions_3_0>
#include <QSharedPointer>
#include <QMatrix4x4>
#include "Triangle.h"
#include <raytracer.h>

class OpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_0 {

public:
    explicit OpenGLWidget(QWidget *parent = nullptr);
    void setGeometry(const QVector<QVector3D> &vertices, const QVector<QVector<int>> &triangleIndices, const QVector<QSharedPointer<triangle>> &triangles);
    QVector3D getCameraPosition() const;
    QVector<QSharedPointer<triangle>> getTriangles() const;
    void updateScene();
    void clearScene();
    rVect QVector3DToRVect(const QVector3D& vec);
    void setRotation(float x, float y, float z);

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

    void calculatePixelOccupation();
    QVector3D projectToScreen(const QVector3D& worldCoord, const QMatrix4x4& modelView, const QMatrix4x4& projection, int windowWidth, int windowHeight);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
};

#endif // OPENGLWIDGET_H
