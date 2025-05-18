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

    // Перечисление для типов фильтрации
    enum FilterType {
        NoFilter = 0,      // Без фильтрации
        ShellFilter = 1,   // Фильтрация оболочки (оптимизация)
        ShadowFilter = 2,  // Фильтрация теневых треугольников
        ResetFilter = 3    // Сброс фильтров
    };

    struct FilterStats {
        int totalTriangles;      // Общее количество треугольников
        int shellTriangles;      // Треугольники в оболочке
        int visibleTriangles;    // Видимые треугольники
        int removedTriangles;    // Удаленные треугольники
        int removedByShell;      // Треугольники, удаленные при оптимизации
        int removedByShadow;     // Треугольники, скрытые при переключении видимости
        FilterType filterType;   // Тип выполненной фильтрации

        // Конструктор по умолчанию
        FilterStats()
            : totalTriangles(0), shellTriangles(0), visibleTriangles(0),
            removedTriangles(0), removedByShell(0), removedByShadow(0),
            filterType(NoFilter) {}

        // Конструктор с параметрами (без filterType)
        FilterStats(int total, int shell, int visible, int removed, int removedShell, int removedShadow)
            : totalTriangles(total), shellTriangles(shell), visibleTriangles(visible),
            removedTriangles(removed), removedByShell(removedShell), removedByShadow(removedShadow),
            filterType(NoFilter) {}
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
