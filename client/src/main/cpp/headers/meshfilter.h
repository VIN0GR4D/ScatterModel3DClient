#ifndef MESHFILTER_H
#define MESHFILTER_H

#include <QVector>
#include <QSharedPointer>
#include "Triangle.h"
#include <unordered_set>
#include <QVector3D>

class MeshFilter {
public:
    struct FilterStats {
        int totalTriangles;
        int shellTriangles;
        int visibleTriangles;
    };

    MeshFilter();
    FilterStats filterMesh(QVector<QSharedPointer<triangle>>& triangles);

private:
    bool isInternalTriangle(const QSharedPointer<triangle>& tri,
                            const QVector<QSharedPointer<triangle>>& triangles);
    bool checkRayIntersection(const QVector3D& origin,
                              const QVector3D& direction,
                              const QSharedPointer<triangle>& tri);
    static const int RAYS_PER_TRIANGLE = 6;
    static const float EPSILON;
};

#endif // MESHFILTER_H
