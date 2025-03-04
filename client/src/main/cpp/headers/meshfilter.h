#ifndef MESHFILTER_H
#define MESHFILTER_H

#include <QVector>
#include <QSharedPointer>
#include "Triangle.h"
#include "rVect.h"
#include "geometryutils.h"

class MeshFilter {
public:
    MeshFilter();
    ~MeshFilter();

    struct FilterStats {
        int totalTriangles;      // Общее количество треугольников
        int shellTriangles;      // Треугольники в оболочке
        int visibleTriangles;    // Видимые треугольники
        int removedTriangles;    // Удаленные треугольники
        int removedByShell;     // Треугольники, удаленные при оптимизации структуры
        int removedByShadow;    // Треугольники, скрытые при переключении видимости
    };

    FilterStats filterMesh(QVector<QSharedPointer<triangle>>& triangles);

private:
    // Структура для представления плоскости сечения
    struct Plane {
        rVect normal;
        double distance;

        Plane(const rVect& n, double d) : normal(n), distance(d) {}
    };

    // Структура для хранения информации о треугольнике в контексте фильтрации
    struct TriangleInfo {
        QSharedPointer<triangle> tri;
        rVect centroid;
        rVect normal;
        bool isShell;

        TriangleInfo(const QSharedPointer<triangle>& t);
    };

    bool isTriangleOnShell(const TriangleInfo& triInfo, const QVector<TriangleInfo>& allTriangles);
    bool isIntersectingPlane(const TriangleInfo& triInfo, const Plane& plane);
    double calculateDistance(const rVect& point, const Plane& plane);
    bool isTriangleVisible(const TriangleInfo& triInfo);
    double calculateObjectSize(const QVector<TriangleInfo>& triangleInfos);
};

#endif // MESHFILTER_H
