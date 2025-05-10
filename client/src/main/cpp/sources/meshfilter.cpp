#include "meshfilter.h"
#include "geometryutils.h"
#include <QDebug>
#include <cmath>
#include <QElapsedTimer>

MeshFilter::MeshFilter() {}

MeshFilter::~MeshFilter() {}

// Конструктор для TriangleInfo с использованием GeometryUtils
MeshFilter::TriangleInfo::TriangleInfo(const QSharedPointer<triangle>& t) : tri(t), isShell(false) {
    // Используем общие методы из GeometryUtils
    centroid = GeometryUtils::calculateCentroid(t);
    normal = GeometryUtils::computeNormal(t);
}

MeshFilter::FilterStats MeshFilter::filterMesh(QVector<QSharedPointer<triangle>>& triangles) {
    QElapsedTimer timer;
    timer.start();

    FilterStats stats = {0, 0, 0, 0, 0, 0};
    stats.totalTriangles = triangles.size();

    if (triangles.isEmpty()) {
        return stats;
    }

    qDebug() << "Starting mesh filtering. Total triangles:" << stats.totalTriangles;

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

    // Рассчитываем размер объекта
    double objectSize = std::sqrt(std::pow(maxX - minX, 2) +
                                  std::pow(maxY - minY, 2) +
                                  std::pow(maxZ - minZ, 2));

    // Создаем пространственную сетку с оптимальным размером ячейки
    SpatialGrid grid;

    // Определяем оптимальный размер ячейки (1% от размера объекта)
    const double DEFAULT_CELL_SIZE_RATIO = 0.01; // 1% от размера объекта
    const double MIN_CELL_SIZE = 1e-6;
    const double MAX_CELL_SIZE = objectSize * 0.1; // не более 10% размера объекта

    grid.cellSize = objectSize * DEFAULT_CELL_SIZE_RATIO;

    // Границы размера ячейки для предотвращения крайних случаев
    if (grid.cellSize < MIN_CELL_SIZE) {
        grid.cellSize = MIN_CELL_SIZE;
    } else if (grid.cellSize > MAX_CELL_SIZE) {
        grid.cellSize = MAX_CELL_SIZE;
    }

    qDebug() << "Spatial grid initialized. Cell size:" << grid.cellSize
             << "Object size:" << objectSize;

    // Заполняем сетку индексами треугольников
    for (int i = 0; i < triangleInfos.size(); ++i) {
        const rVect& center = triangleInfos[i].centroid;

        MeshFilterCellKey key;
        key.x = static_cast<qint64>(std::floor(center.getX() / grid.cellSize));
        key.y = static_cast<qint64>(std::floor(center.getY() / grid.cellSize));
        key.z = static_cast<qint64>(std::floor(center.getZ() / grid.cellSize));

        grid.cells[key].append(i);
    }

    qDebug() << "Grid populated with" << grid.cells.size() << "cells";

    // Выполняем фильтрацию с оптимизированным сохранением результатов
#ifdef _OPENMP
#pragma omp parallel for if(triangleInfos.size() > 1000)
#endif
    for (int i = 0; i < triangleInfos.size(); ++i) {
        triangleInfos[i].isShell = isTriangleOnShell(i, triangleInfos, grid);
    }

    // Формируем финальный список треугольников
    for (int i = 0; i < triangleInfos.size(); ++i) {
        if (triangleInfos[i].isShell) {
            stats.shellTriangles++;
            shellTriangles.append(triangleInfos[i].tri);
        }
        if (triangleInfos[i].tri->getVisible()) {
            stats.visibleTriangles++;
        }
    }

    // Эффективная замена вектора треугольников
    stats.removedByShell = triangles.size() - shellTriangles.size();
    triangles = std::move(shellTriangles);

    qDebug() << "Filtering completed in" << timer.elapsed() << "ms:";
    qDebug() << "Total triangles:" << stats.totalTriangles;
    qDebug() << "Shell triangles:" << stats.shellTriangles;
    qDebug() << "Visible triangles:" << stats.visibleTriangles;
    qDebug() << "Removed triangles:" << stats.removedByShell;

    return stats;
}

bool MeshFilter::isTriangleOnShell(int triIndex, const QVector<TriangleInfo>& allTriangles, const SpatialGrid& grid) {
    const double EPSILON = 1e-6;
    const TriangleInfo& triInfo = allTriangles[triIndex];
    const rVect& center = triInfo.centroid;

    // Находим ячейку сетки для текущего треугольника
    MeshFilterCellKey key;
    key.x = static_cast<qint64>(std::floor(center.getX() / grid.cellSize));
    key.y = static_cast<qint64>(std::floor(center.getY() / grid.cellSize));
    key.z = static_cast<qint64>(std::floor(center.getZ() / grid.cellSize));

    // Проверяем соседние ячейки в радиусе
    int adjacentCount = 0;
    rVect triNormal = triInfo.normal;

    // Перебираем ячейки в радиусе вокруг текущей
    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dz = -1; dz <= 1; ++dz) {
                MeshFilterCellKey neighKey;
                neighKey.x = key.x + dx;
                neighKey.y = key.y + dy;
                neighKey.z = key.z + dz;

                // Проверяем только треугольники из соседних ячеек
                const QVector<int>& neighbors = grid.cells.value(neighKey);
                for (int neighborIdx : neighbors) {
                    // Пропускаем сам треугольник
                    if (neighborIdx == triIndex) continue;

                    const TriangleInfo& otherTriInfo = allTriangles[neighborIdx];

                    // Вычисляем вектор между центроидами
                    rVect centroidVector = otherTriInfo.centroid - triInfo.centroid;
                    double distance = centroidVector.length();

                    // Проверяем близость треугольников
                    if (distance < grid.cellSize * 1.5) { // Немного больше размера ячейки для надежности
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
