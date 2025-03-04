#ifndef RAYTRACER_H
#define RAYTRACER_H

#include "rVect.h"
#include "Triangle.h"
#include "geometryutils.h"
#include <QVector>
#include <QSharedPointer>

class RayTracer {
public:
    RayTracer();
    ~RayTracer();
    void setTriangles(const QVector<QSharedPointer<triangle>>& tris);
    bool intersects(const rVect& rayOrigin, const rVect& rayVector, const triangle& tri, double& t, double& u, double& v);
    void determineVisibility(const QVector<QSharedPointer<triangle>>& triangles, const rVect& observerPosition);
    QVector<QSharedPointer<triangle>> visibleTriangles;

private:
    QVector<QSharedPointer<triangle>> triangles;
};

#endif // RAYTRACER_H
