#include "ProjectSerializer.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

// Метод для сохранения проекта в файл
bool ProjectSerializer::saveProject(const QString& fileName, const ProjectData& data) {
    QJsonObject rootObject; // Корневой объект JSON

    // Сохранение треугольников
    QJsonArray trianglesArray; // Массив для хранения треугольников
    for (const auto& tri : data.triangles) { // Проход по всем треугольникам в данных проекта
        QJsonObject triObject; // Объект для текущего треугольника

        // Сохранение координат вершин треугольника
        triObject["v1"] = QJsonArray{tri->getV1()->getX(), tri->getV1()->getY(), tri->getV1()->getZ()};
        triObject["v2"] = QJsonArray{tri->getV2()->getX(), tri->getV2()->getY(), tri->getV2()->getZ()};
        triObject["v3"] = QJsonArray{tri->getV3()->getX(), tri->getV3()->getY(), tri->getV3()->getZ()};

        // Сохранение видимости треугольника
        triObject["visible"] = tri->getVisible();

        // Добавление объекта треугольника в массив треугольников
        trianglesArray.append(triObject);
    }
    // Добавление массива треугольников в корневой объект
    rootObject["triangles"] = trianglesArray;

    // Сохранение позиции объекта
    QJsonObject positionObject; // Объект для позиции
    positionObject["x"] = data.objectPosition.x();
    positionObject["y"] = data.objectPosition.y();
    positionObject["z"] = data.objectPosition.z();
    rootObject["objectPosition"] = positionObject; // Добавление позиции в корневой объект

    // Сохранение параметров подстилающей поверхности
    rootObject["showUnderlyingSurface"] = data.showUnderlyingSurface;
    rootObject["surfaceAlphaValue"] = data.surfaceAlphaValue;

    // Сохранение параметров поворота
    QJsonObject rotationObject; // Объект для поворота
    rotationObject["x"] = data.rotationX;
    rotationObject["y"] = data.rotationY;
    rotationObject["z"] = data.rotationZ;
    rootObject["rotation"] = rotationObject; // Добавление поворота в корневой объект

    // Сохранение коэффициентов масштабирования
    QJsonObject scalingObject; // Объект для масштабирования
    scalingObject["x"] = data.scalingCoefficients.x();
    scalingObject["y"] = data.scalingCoefficients.y();
    scalingObject["z"] = data.scalingCoefficients.z();
    rootObject["scaling"] = scalingObject; // Добавление масштабирования в корневой объект

    // Сохранение результатов расчетов для absEout
    QJsonArray absEoutArray; // Массив для absEout
    for (const double& value : data.absEout) { // Проход по всем значениям absEout
        absEoutArray.append(value); // Добавление значения в массив
    }
    rootObject["absEout"] = absEoutArray; // Добавление массива в корневой объект

    // для normEout
    QJsonArray normEoutArray;
    for (const double& value : data.normEout) {
        normEoutArray.append(value);
    }
    rootObject["normEout"] = normEoutArray; // Добавление массива в корневой объект

    // Сохранение двумерного массива absEout2D
    QJsonArray absEout2DArray; // Массив для absEout2D
    for (const auto& vec : data.absEout2D) { // Проход по всем строкам двумерного массива
        QJsonArray innerArray; // Внутренний массив для строки
        for (const double& value : vec) { // Проход по всем значениям в строке
            innerArray.append(value); // Добавление значения во внутренний массив
        }
        absEout2DArray.append(innerArray); // Добавление строки в двумерный массив
    }
    rootObject["absEout2D"] = absEout2DArray; // Добавление двумерного массива в корневой объект

    // для массива normEout2D
    QJsonArray normEout2DArray;
    for (const auto& vec : data.normEout2D) {
        QJsonArray innerArray;
        for (const double& value : vec) {
            innerArray.append(value);
        }
        normEout2DArray.append(innerArray);
    }
    rootObject["normEout2D"] = normEout2DArray;

    // Сохранение сохраненных результатов
    rootObject["storedResults"] = data.storedResults; // Добавление строки с результатами в корневой объект

    rootObject["projectName"] = data.projectName;
    rootObject["workingDirectory"] = data.workingDirectory;

    // Создание документа JSON из корневого объекта
    QJsonDocument doc(rootObject);

    // Открытие файла для записи
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) { // Проверка успешности открытия файла
        return false; //
    }

    // Запись JSON-документа в файл
    file.write(doc.toJson());
    file.close();
    return true;// Возврат true при успешном сохранении
}

// Метод для загрузки проекта из файла
bool ProjectSerializer::loadProject(const QString& fileName, ProjectData& data) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) { // Попытка открыть файл для чтения
        return false;
    }

    QByteArray fileData = file.readAll(); // Чтение всех данных из файла
    file.close();

    // Создание JSON-документа из считанных данных
    QJsonDocument doc(QJsonDocument::fromJson(fileData));
    if (doc.isNull() || !doc.isObject()) { // Проверка корректности JSON
        return false;
    }

    QJsonObject rootObject = doc.object(); // Получение корневого объекта JSON

    // Загрузка треугольников из JSON
    data.triangles.clear(); // Очистка текущего списка треугольников
    QJsonArray trianglesArray = rootObject["triangles"].toArray(); // Получение массива треугольников
    for (const QJsonValue& triValue : trianglesArray) { // Проход по каждому треугольнику в массиве
        QJsonObject triObject = triValue.toObject(); // Получение объекта треугольника

        // Получение массивов координат вершин
        QJsonArray v1Array = triObject["v1"].toArray();
        QJsonArray v2Array = triObject["v2"].toArray();
        QJsonArray v3Array = triObject["v3"].toArray();

        // Проверка корректности размеров массивов координат
        if (v1Array.size() != 3 || v2Array.size() != 3 || v3Array.size() != 3) {
            continue; // Пропуск некорректных данных
        }

        // Создание векторов для каждой вершины
        QSharedPointer<rVect> v1 = QSharedPointer<rVect>::create(
            v1Array[0].toDouble(), v1Array[1].toDouble(), v1Array[2].toDouble());
        QSharedPointer<rVect> v2 = QSharedPointer<rVect>::create(
            v2Array[0].toDouble(), v2Array[1].toDouble(), v2Array[2].toDouble());
        QSharedPointer<rVect> v3 = QSharedPointer<rVect>::create(
            v3Array[0].toDouble(), v3Array[1].toDouble(), v3Array[2].toDouble());

        // Получение видимости треугольника, по умолчанию true
        bool visible = triObject["visible"].toBool(true);

        // Создание узлов из векторов
        QSharedPointer<node> node1 = QSharedPointer<node>::create(v1->getX(), v1->getY(), v1->getZ());
        QSharedPointer<node> node2 = QSharedPointer<node>::create(v2->getX(), v2->getY(), v2->getZ());
        QSharedPointer<node> node3 = QSharedPointer<node>::create(v3->getX(), v3->getY(), v3->getZ());

        // Создание треугольника с параметром 'visible' и добавление его в список треугольников
        QSharedPointer<triangle> tri = QSharedPointer<triangle>::create(visible, node1, node2, node3);
        data.triangles.append(tri);
    }

    // Загрузка позиции объекта из JSON
    QJsonObject positionObject = rootObject["objectPosition"].toObject(); // Получение объекта позиции
    data.objectPosition.setX(positionObject["x"].toDouble());
    data.objectPosition.setY(positionObject["y"].toDouble());
    data.objectPosition.setZ(positionObject["z"].toDouble());

    // Загрузка параметров подстилающей поверхности
    data.showUnderlyingSurface = rootObject["showUnderlyingSurface"].toBool(false);
    data.surfaceAlphaValue = rootObject["surfaceAlphaValue"].toDouble(0.3);

    // Загрузка параметров поворота из JSON
    QJsonObject rotationObject = rootObject["rotation"].toObject(); // Получение объекта поворота
    data.rotationX = rotationObject["x"].toDouble();
    data.rotationY = rotationObject["y"].toDouble();
    data.rotationZ = rotationObject["z"].toDouble();

    // Загрузка коэффициентов масштабирования из JSON
    QJsonObject scalingObject = rootObject["scaling"].toObject(); // Получение объекта масштабирования
    data.scalingCoefficients.setX(scalingObject["x"].toDouble());
    data.scalingCoefficients.setY(scalingObject["y"].toDouble());
    data.scalingCoefficients.setZ(scalingObject["z"].toDouble());

    // Загрузка результатов расчетов для absEout из JSON
    data.absEout.clear();
    QJsonArray absEoutArray = rootObject["absEout"].toArray();
    for (const QJsonValue& value : absEoutArray) {
        data.absEout.append(value.toDouble());
    }

    // Загрузка результатов расчетов для normEout из JSON
    data.normEout.clear();
    QJsonArray normEoutArray = rootObject["normEout"].toArray();
    for (const QJsonValue& value : normEoutArray) {
        data.normEout.append(value.toDouble());
    }

    // Загрузка двумерного массива absEout2D из JSON
    data.absEout2D.clear();
    QJsonArray absEout2DArray = rootObject["absEout2D"].toArray(); // Получение массива absEout2D
    for (const QJsonValue& rowValue : absEout2DArray) { // Проход по каждой строке массива
        QJsonArray rowArray = rowValue.toArray(); // Получение строки как массива
        QVector<double> row; // Вектор для хранения значений строки
        for (const QJsonValue& value : rowArray) { // Проход по всем значениям в строке
            row.append(value.toDouble()); // Добавление значения в строку
        }
        data.absEout2D.append(row); // Добавление строки в двумерный массив
    }

    // Загрузка двумерного массива normEout2D из JSON
    data.normEout2D.clear();
    QJsonArray normEout2DArray = rootObject["normEout2D"].toArray();
    for (const QJsonValue& rowValue : normEout2DArray) {
        QJsonArray rowArray = rowValue.toArray();
        QVector<double> row;
        for (const QJsonValue& value : rowArray) {
            row.append(value.toDouble());
        }
        data.normEout2D.append(row);
    }

    // Загрузка сохраненных результатов из JSON
    data.storedResults = rootObject["storedResults"].toString(); // Установка строки с результатами

    data.projectName = rootObject["projectName"].toString();
    data.workingDirectory = rootObject["workingDirectory"].toString();

    return true; // Возврат true при успешной загрузке
}
