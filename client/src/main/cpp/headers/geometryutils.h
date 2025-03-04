#ifndef GEOMETRYUTILS_H
#define GEOMETRYUTILS_H

#include "Triangle.h"
#include "rVect.h"
#include <QSharedPointer>

/**
 * @brief Класс с утилитами для геометрических вычислений
 */
class GeometryUtils {
public:
    /**
     * @brief Вычисляет нормаль треугольника
     * @param tri Указатель на треугольник
     * @return Нормализованный вектор нормали
     */
    static rVect computeNormal(const QSharedPointer<const triangle>& tri);

    /**
     * @brief Вычисляет центроид (центр масс) треугольника
     * @param tri Указатель на треугольник
     * @return Вектор положения центроида
     */
    static rVect calculateCentroid(const QSharedPointer<const triangle>& tri);

    /**
     * @brief Проверяет, направлен ли треугольник к наблюдателю
     * @param normal Нормаль треугольника
     * @param observerToTriangle Вектор от наблюдателя к треугольнику
     * @return true если треугольник обращен к наблюдателю
     */
    static bool isFacingObserver(const rVect& normal, const rVect& observerToTriangle);
};

#endif // GEOMETRYUTILS_H
