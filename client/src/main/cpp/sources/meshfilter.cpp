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

    // Предварительное выделение памяти для векторов
    QVector<TriangleInfo> triangleInfos;
    triangleInfos.reserve(triangles.size());
    QVector<QSharedPointer<triangle>> shellTriangles;
    shellTriangles.reserve(triangles.size());

    // Создаем массив информации о треугольниках
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

    // Выполняем фильтрацию с оптимизированным сохранением результатов
    for (auto& triInfo : triangleInfos) {
        triInfo.isShell = isTriangleOnShell(triInfo, triangleInfos);
        if (triInfo.isShell) {
            stats.shellTriangles++;
            shellTriangles.append(triInfo.tri);
        }
        if (triInfo.tri->getVisible()) {
            stats.visibleTriangles++;
        }
    }

    // Эффективная замена вектора треугольников
    stats.removedByShell = triangles.size() - shellTriangles.size();
    triangles = std::move(shellTriangles);

    qDebug() << "Filtering completed:";
    qDebug() << "Total triangles:" << stats.totalTriangles;
    qDebug() << "Shell triangles:" << stats.shellTriangles;
    qDebug() << "Visible triangles:" << stats.visibleTriangles;
    qDebug() << "Removed triangles:" << stats.removedByShell;

    return stats;
}

bool MeshFilter::isTriangleOnShell(const TriangleInfo& triInfo, const QVector<TriangleInfo>& allTriangles) {
    const double EPSILON = 1e-6;
    static double cachedObjectSize = 0.0;
    static double cachedMaxDistance = 0.0;
    cachedObjectSize = 0.0; // Сброс кэша для нового расчета

    // Вычисляем MAX_DISTANCE только один раз для всего набора треугольников
    if (cachedObjectSize == 0.0) {
        cachedObjectSize = calculateObjectSize(allTriangles);
        cachedMaxDistance = cachedObjectSize * 0.01; // 1% от размера объекта
    }

    int adjacentCount = 0;
    rVect triNormal = triInfo.normal;

    for (const auto& otherTriInfo : allTriangles) {
        if (&otherTriInfo == &triInfo) continue;

        // Вычисляем вектор между центроидами
        rVect centroidVector = otherTriInfo.centroid - triInfo.centroid;
        double distance = centroidVector.length();

        // Проверяем близость треугольников
        if (distance < cachedMaxDistance) {
            // Проверяем угол между нормалями
            double dotProduct = triNormal * otherTriInfo.normal;
            // Проверяем параллельность и противоположную направленность
            if (std::abs(dotProduct + 1.0) < EPSILON) {
                adjacentCount++;
                // Оптимизация: если уже нашли достаточно соседей, можно выходить
                if (adjacentCount > 2) return false;
            }
        }
    }

    // Если треугольник имеет мало параллельных соседей, считаем его частью оболочки
    return adjacentCount <= 2;
}

double MeshFilter::calculateDistance(const rVect& point, const Plane& plane) {
    return point * plane.normal - plane.distance;
}

double MeshFilter::calculateObjectSize(const QVector<TriangleInfo>& triangleInfos) {
    if (triangleInfos.isEmpty()) return 0.0;

    // Используем уже существующий код определения границ
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

    return std::sqrt(std::pow(maxX - minX, 2) +
                     std::pow(maxY - minY, 2) +
                     std::pow(maxZ - minZ, 2));
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
