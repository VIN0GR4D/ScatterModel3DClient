#include "geometryutils.h"

rVect GeometryUtils::computeNormal(const QSharedPointer<const triangle>& tri) {
    const rVect v1 = *tri->getV2() - *tri->getV1();
    const rVect v2 = *tri->getV3() - *tri->getV1();
    rVect normal = v1 ^ v2;
    normal.normalize();
    return normal;
}

// Функция для вычисления центроида треугольника
rVect GeometryUtils::calculateCentroid(const QSharedPointer<const triangle>& tri) {
    const rVect& v1 = *tri->getV1();
    const rVect& v2 = *tri->getV2();
    const rVect& v3 = *tri->getV3();

    return rVect(
        (v1.getX() + v2.getX() + v3.getX()) / 3.0,
        (v1.getY() + v2.getY() + v3.getY()) / 3.0,
        (v1.getZ() + v2.getZ() + v3.getZ()) / 3.0
        );
}

bool GeometryUtils::isFacingObserver(const rVect& normal, const rVect& observerToTriangle) {
    double dotProduct = normal * observerToTriangle;
    return dotProduct > 0;
}
