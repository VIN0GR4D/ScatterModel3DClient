#ifndef MESHFILTER_H
#define MESHFILTER_H

#include <QVector>
#include <QSharedPointer>
#include <QHash>
#include "Triangle.h"
#include "rVect.h"
#include "geometryutils.h"

// Ключ для пространственной хеш-таблицы - объявляем вне класса MeshFilter
struct MeshFilterCellKey {
    qint64 x, y, z;

    bool operator==(const MeshFilterCellKey& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

// Определение хеш-функции для CellKey для использования в QHash
inline uint qHash(const MeshFilterCellKey& key, uint seed = 0) {
    return qHash(key.x, seed) ^ qHash(key.y, seed) ^ qHash(key.z, seed);
}

class MeshFilter {
public:
    MeshFilter();
    ~MeshFilter();

    struct FilterStats {
        int totalTriangles;      // Общее количество треугольников
        int shellTriangles;      // Треугольники в оболочке
        int visibleTriangles;    // Видимые треугольники
        int removedTriangles;    // Удаленные треугольники
        int removedByShell;      // Треугольники, удаленные при оптимизации структуры
        int removedByShadow;     // Треугольники, скрытые при переключении видимости
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

    // Структура для пространственного разбиения
    struct SpatialGrid {
        QHash<MeshFilterCellKey, QVector<int>> cells;
        double cellSize;
    };

    bool isTriangleOnShell(int triIndex, const QVector<TriangleInfo>& allTriangles, const SpatialGrid& grid);
    bool isIntersectingPlane(const TriangleInfo& triInfo, const Plane& plane);
    double calculateDistance(const rVect& point, const Plane& plane);
    double calculateObjectSize(const QVector<TriangleInfo>& triangleInfos);
};

#endif // MESHFILTER_H
