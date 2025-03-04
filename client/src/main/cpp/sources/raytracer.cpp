#include "raytracer.h"
#include <QDebug>
#include <QMutexLocker>

RayTracer::RayTracer() {}

RayTracer::~RayTracer() {
}

// Установка треугольников для трассировки лучей
void RayTracer::setTriangles(const QVector<QSharedPointer<triangle>>& tris) {
    triangles = tris; // Сохранение списка треугольников в поле класса
}

// Функция для получения компоненты вектора по индексу (оставлена для обратной совместимости)
double getComponent(const rVect& vec, int index) {
    switch (index) {
    case 0: return vec.getX();
    case 1: return vec.getY();
    case 2: return vec.getZ();
    default: throw std::out_of_range("Индекс вне диапазона для доступа к компоненту rVect");
    }
}

// Проверка пересечения луча с треугольником
bool RayTracer::intersects(const rVect& rayOrigin, const rVect& rayVector, const triangle& tri, double& t, double& u, double& v) {
    const double EPSILON = 0.0000001;
    const rVect vertex0 = *tri.getV1();
    const rVect vertex1 = *tri.getV2();
    const rVect vertex2 = *tri.getV3();
    const rVect edge1 = vertex1 - vertex0;
    const rVect edge2 = vertex2 - vertex0;
    const rVect h = rayVector ^ edge2;
    const double a = edge1 * h;

    if (std::abs(a) < EPSILON) {
        return false; // Луч параллелен треугольнику
    }

    const double f = 1.0 / a;
    const rVect s = rayOrigin - vertex0;
    u = f * (s * h);

    if (u < 0.0 || u > 1.0) {
        return false;
    }

    const rVect q = s ^ edge1;
    v = f * (rayVector * q);

    if (v < 0.0 || u + v > 1.0) {
        return false;
    }

    t = f * (edge2 * q);
    return t > EPSILON; // Пересечение произошло, если t > EPSILON
}

// Определение видимости треугольников относительно наблюдателя
void RayTracer::determineVisibility(const QVector<QSharedPointer<triangle>>& triangles, const rVect& observerPosition) {
    for (const auto& triPtr : triangles) {
        const rVect normal = GeometryUtils::computeNormal(triPtr);
        const rVect centroid = GeometryUtils::calculateCentroid(triPtr);
        const rVect observerToTriangle = centroid - observerPosition;
        bool isVisible = GeometryUtils::isFacingObserver(normal, observerToTriangle);
        triPtr->setVisible(isVisible); // Установка флага видимости для треугольника
    }
}

//     for (auto& triPtr : triangles) {
//         triPtr->setVisible(true);
//     }
// }
