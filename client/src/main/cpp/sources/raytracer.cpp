#include "raytracer.h"
#include <QDebug>
#include <QMutexLocker>

RayTracer::RayTracer() {}

RayTracer::~RayTracer() {
}

// Функция для вычисления центроида треугольника
rVect calculateCentroid(const QSharedPointer<const triangle>& tri) {
    const rVect v1 = *tri->getV1();
    const rVect v2 = *tri->getV2();
    const rVect v3 = *tri->getV3();
    return rVect((v1.getX() + v2.getX() + v3.getX()) / 3,
                 (v1.getY() + v2.getY() + v3.getY()) / 3,
                 (v1.getZ() + v2.getZ() + v3.getZ()) / 3);
}

// Установка треугольников для трассировки лучей
void RayTracer::setTriangles(const QVector<QSharedPointer<triangle>>& tris) {
    triangles = tris; // Сохранение списка треугольников в поле класса
}

// Функция для получения компоненты вектора по индексу
double getComponent(const rVect& vec, int index) {
    switch (index) {
    case 0: return vec.getX();
    case 1: return vec.getY();
    case 2: return vec.getZ();
    default: throw std::out_of_range("Index out of range for rVect component access");
    }
}

// Вычисление нормали треугольника
rVect RayTracer::computeNormal(const QSharedPointer<const triangle>& tri) {
    const rVect v1 = *tri->getV2() - *tri->getV1();
    const rVect v2 = *tri->getV3() - *tri->getV1();
    rVect normal = v1 ^ v2;
    normal.normalize();
    return normal;
}

// Проверка, направлен ли треугольник к наблюдателю
bool RayTracer::isFacingObserver(const rVect& normal, const rVect& observerToTriangle) {
    double dotProduct = normal * observerToTriangle;
    return dotProduct > 0;
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
        const rVect normal = computeNormal(triPtr);
        const rVect observerToTriangle = calculateCentroid(triPtr) - observerPosition;
        bool isVisible = isFacingObserver(normal, observerToTriangle);
        triPtr->setVisible(isVisible); // Установка флага видимости для треугольника
    }
}

//     for (auto& triPtr : triangles) {
//         triPtr->setVisible(true);
//     }
// }
