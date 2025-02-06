#include "meshfilter.h"
#include <QDebug>

MeshFilter::MeshFilter() {}

MeshFilter::~MeshFilter() {}

// Конструктор для TriangleInfo
MeshFilter::TriangleInfo::TriangleInfo(const QSharedPointer<triangle>& t) : tri(t), isShell(false) {
    // Вычисляем центроид треугольника
    const rVect& v1 = *t->getV1();
    const rVect& v2 = *t->getV2();
    const rVect& v3 = *t->getV3();

    centroid = rVect(
        (v1.getX() + v2.getX() + v3.getX()) / 3.0,
        (v1.getY() + v2.getY() + v3.getY()) / 3.0,
        (v1.getZ() + v2.getZ() + v3.getZ()) / 3.0
        );

    // Вычисляем нормаль треугольника
    rVect edge1 = *t->getV2() - *t->getV1();
    rVect edge2 = *t->getV3() - *t->getV1();
    normal = edge1 ^ edge2;
    normal.normalize();
}

MeshFilter::FilterStats MeshFilter::filterMesh(QVector<QSharedPointer<triangle>>& triangles) {
    FilterStats stats = {0, 0, 0, 0, 0};
    stats.totalTriangles = triangles.size();

    if (triangles.isEmpty()) {
        return stats;
    }

    // Создаем массив информации о треугольниках
    QVector<TriangleInfo> triangleInfos;
    triangleInfos.reserve(triangles.size());
    for (const auto& tri : triangles) {
        triangleInfos.append(TriangleInfo(tri));
    }

    // Определяем границы объекта
    double minX = triangleInfos[0].centroid.getX();
    double maxX = minX;
    double minY = triangleInfos[0].centroid.getY();
    double maxY = minY;
    double minZ = triangleInfos[0].centroid.getZ();
    double maxZ = minZ;

    for (const auto& triInfo : triangleInfos) {
        minX = qMin(minX, triInfo.centroid.getX());
        maxX = qMax(maxX, triInfo.centroid.getX());
        minY = qMin(minY, triInfo.centroid.getY());
        maxY = qMax(maxY, triInfo.centroid.getY());
        minZ = qMin(minZ, triInfo.centroid.getZ());
        maxZ = qMax(maxZ, triInfo.centroid.getZ());
    }

    // Создаем основные плоскости сечения
    QVector<Plane> planes = {
        Plane(rVect(1, 0, 0), (minX + maxX) / 2), // YZ плоскость
        Plane(rVect(0, 1, 0), (minY + maxY) / 2), // XZ плоскость
        Plane(rVect(0, 0, 1), (minZ + maxZ) / 2)  // XY плоскость
    };

    // Выполняем фильтрацию
    for (auto& triInfo : triangleInfos) {
        triInfo.isShell = isTriangleOnShell(triInfo, triangleInfos);
        if (triInfo.isShell) {
            stats.shellTriangles++;
        }
        if (triInfo.tri->getVisible()) {
            stats.visibleTriangles++;
        }
    }

    // Обновляем состояние треугольников
    int originalCount = triangles.size();
    triangles.clear();
    for (const auto& triInfo : triangleInfos) {
        if (triInfo.isShell) {
            triangles.append(triInfo.tri);
        }
    }
    stats.removedByShell = originalCount - triangles.size();

    qDebug() << "Filtering completed:";
    qDebug() << "Total triangles:" << stats.totalTriangles;
    qDebug() << "Shell triangles:" << stats.shellTriangles;
    qDebug() << "Visible triangles:" << stats.visibleTriangles;

    return stats;
}

bool MeshFilter::isTriangleOnShell(const TriangleInfo& triInfo, const QVector<TriangleInfo>& allTriangles) {
    const double EPSILON = 1e-6;
    const double MAX_DISTANCE = 0.1; // Максимальное расстояние для определения близости треугольников

    int adjacentCount = 0;
    rVect triNormal = triInfo.normal;

    for (const auto& otherTriInfo : allTriangles) {
        if (&otherTriInfo == &triInfo) continue;

        // Вычисляем вектор между центроидами
        rVect centroidVector = otherTriInfo.centroid - triInfo.centroid;
        double distance = centroidVector.length();

        // Проверяем близость треугольников
        if (distance < MAX_DISTANCE) {
            // Проверяем угол между нормалями
            double dotProduct = triNormal * otherTriInfo.normal;
            if (std::abs(dotProduct + 1.0) < EPSILON) {
                // Треугольники почти параллельны и противоположно направлены
                adjacentCount++;
            }
        }
    }

    // Если треугольник имеет мало параллельных соседей, считаем его частью оболочки
    return adjacentCount <= 2;
}

double MeshFilter::calculateDistance(const rVect& point, const Plane& plane) {
    return point * plane.normal - plane.distance;
}

bool MeshFilter::isIntersectingPlane(const TriangleInfo& triInfo, const Plane& plane) {
    const double EPSILON = 1e-6;

    double d1 = calculateDistance(*triInfo.tri->getV1(), plane);
    double d2 = calculateDistance(*triInfo.tri->getV2(), plane);
    double d3 = calculateDistance(*triInfo.tri->getV3(), plane);

    // Треугольник пересекает плоскость, если расстояния имеют разные знаки
    return (d1 * d2 < 0) || (d2 * d3 < 0) || (d3 * d1 < 0) ||
           (std::abs(d1) < EPSILON) || (std::abs(d2) < EPSILON) || (std::abs(d3) < EPSILON);
}
