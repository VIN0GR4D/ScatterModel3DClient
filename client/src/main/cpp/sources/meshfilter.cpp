#include "meshfilter.h"
#include <algorithm>
#include <random>

const float MeshFilter::EPSILON = 0.0001f;

MeshFilter::MeshFilter() {}

MeshFilter::FilterStats MeshFilter::filterMesh(QVector<QSharedPointer<triangle>>& triangles) {
    FilterStats stats;
    stats.totalTriangles = triangles.size();

    // Временный вектор для хранения треугольников оболочки
    QVector<QSharedPointer<triangle>> shellTriangles;

    // Фильтрация внутренних треугольников
    for (const auto& tri : triangles) {
        if (!isInternalTriangle(tri, triangles)) {
            shellTriangles.push_back(tri);
        }
    }

    // Обновление статистики
    stats.shellTriangles = shellTriangles.size();
    stats.visibleTriangles = std::count_if(shellTriangles.begin(), shellTriangles.end(),
                                           [](const QSharedPointer<triangle>& tri) { return tri->getVisible(); });

    // Замена исходного вектора отфильтрованным
    triangles = shellTriangles;

    return stats;
}

bool MeshFilter::isInternalTriangle(const QSharedPointer<triangle>& tri,
                                    const QVector<QSharedPointer<triangle>>& triangles) {
    // Вычисление центра треугольника
    QVector3D center(
        (tri->getV1()->getX() + tri->getV2()->getX() + tri->getV3()->getX()) / 3.0f,
        (tri->getV1()->getY() + tri->getV2()->getY() + tri->getV3()->getY()) / 3.0f,
        (tri->getV1()->getZ() + tri->getV2()->getZ() + tri->getV3()->getZ()) / 3.0f
        );

    // Генерация случайных направлений для лучей
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);

    int intersectionCount = 0;

    for (int i = 0; i < RAYS_PER_TRIANGLE; ++i) {
        // Генерация случайного направления
        QVector3D direction(dis(gen), dis(gen), dis(gen));
        direction.normalize();

        // Подсчёт пересечений луча с другими треугольниками
        int currentIntersections = 0;
        for (const auto& otherTri : triangles) {
            if (otherTri != tri && checkRayIntersection(center, direction, otherTri)) {
                currentIntersections++;
            }
        }

        // Если число пересечений нечётное, увеличиваем счётчик
        if (currentIntersections % 2 == 1) {
            intersectionCount++;
        }
    }

    // Треугольник считается внутренним, если большинство лучей имеет нечётное число пересечений
    return intersectionCount > RAYS_PER_TRIANGLE / 2;
}

bool MeshFilter::checkRayIntersection(const QVector3D& origin,
                                      const QVector3D& direction,
                                      const QSharedPointer<triangle>& tri) {
    QVector3D v0(tri->getV1()->getX(), tri->getV1()->getY(), tri->getV1()->getZ());
    QVector3D v1(tri->getV2()->getX(), tri->getV2()->getY(), tri->getV2()->getZ());
    QVector3D v2(tri->getV3()->getX(), tri->getV3()->getY(), tri->getV3()->getZ());

    // Вычисление нормали треугольника
    QVector3D edge1 = v1 - v0;
    QVector3D edge2 = v2 - v0;
    QVector3D normal = QVector3D::crossProduct(edge1, edge2).normalized();

    // Проверка на параллельность луча и треугольника
    float dot = QVector3D::dotProduct(direction, normal);
    if (std::abs(dot) < EPSILON) {
        return false;
    }

    // Вычисление расстояния от начала луча до плоскости треугольника
    float d = -QVector3D::dotProduct(normal, v0);
    float t = -(QVector3D::dotProduct(normal, origin) + d) / dot;

    // Если пересечение позади начала луча, игнорируем его
    if (t < EPSILON) {
        return false;
    }

    // Вычисление точки пересечения
    QVector3D intersection = origin + direction * t;

    // Проверка, находится ли точка внутри треугольника
    QVector3D c1 = QVector3D::crossProduct(edge1, intersection - v0);
    QVector3D c2 = QVector3D::crossProduct(v2 - v1, intersection - v1);
    QVector3D c3 = QVector3D::crossProduct(v0 - v2, intersection - v2);

    bool inside = QVector3D::dotProduct(normal, c1) > 0 &&
                  QVector3D::dotProduct(normal, c2) > 0 &&
                  QVector3D::dotProduct(normal, c3) > 0;

    return inside;
}
