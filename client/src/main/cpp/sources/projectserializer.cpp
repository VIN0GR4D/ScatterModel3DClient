#include "ProjectSerializer.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

bool ProjectSerializer::saveProject(const QString& fileName, const ProjectData& data) {
    QJsonObject rootObject;

    // Saving triangles
    QJsonArray trianglesArray;
    for (const auto& tri : data.triangles) {
        QJsonObject triObject;
        triObject["v1"] = QJsonArray{tri->getV1()->getX(), tri->getV1()->getY(), tri->getV1()->getZ()};
        triObject["v2"] = QJsonArray{tri->getV2()->getX(), tri->getV2()->getY(), tri->getV2()->getZ()};
        triObject["v3"] = QJsonArray{tri->getV3()->getX(), tri->getV3()->getY(), tri->getV3()->getZ()};
        triObject["visible"] = tri->getVisible();
        trianglesArray.append(triObject);
    }
    rootObject["triangles"] = trianglesArray;

    // Saving object position
    QJsonObject positionObject;
    positionObject["x"] = data.objectPosition.x();
    positionObject["y"] = data.objectPosition.y();
    positionObject["z"] = data.objectPosition.z();
    rootObject["objectPosition"] = positionObject;

    // Saving rotation
    QJsonObject rotationObject;
    rotationObject["x"] = data.rotationX;
    rotationObject["y"] = data.rotationY;
    rotationObject["z"] = data.rotationZ;
    rootObject["rotation"] = rotationObject;

    // Saving scaling coefficients
    QJsonObject scalingObject;
    scalingObject["x"] = data.scalingCoefficients.x();
    scalingObject["y"] = data.scalingCoefficients.y();
    scalingObject["z"] = data.scalingCoefficients.z();
    rootObject["scaling"] = scalingObject;

    // Saving calculation results
    // For absEout
    QJsonArray absEoutArray;
    for (const double& value : data.absEout) {
        absEoutArray.append(value);
    }
    rootObject["absEout"] = absEoutArray;

    // For normEout
    QJsonArray normEoutArray;
    for (const double& value : data.normEout) {
        normEoutArray.append(value);
    }
    rootObject["normEout"] = normEoutArray;

    // Saving 2D arrays
    QJsonArray absEout2DArray;
    for (const auto& vec : data.absEout2D) {
        QJsonArray innerArray;
        for (const double& value : vec) {
            innerArray.append(value);
        }
        absEout2DArray.append(innerArray);
    }
    rootObject["absEout2D"] = absEout2DArray;

    QJsonArray normEout2DArray;
    for (const auto& vec : data.normEout2D) {
        QJsonArray innerArray;
        for (const double& value : vec) {
            innerArray.append(value);
        }
        normEout2DArray.append(innerArray);
    }
    rootObject["normEout2D"] = normEout2DArray;

    // Saving storedResults
    rootObject["storedResults"] = data.storedResults;

    // Writing to file
    QJsonDocument doc(rootObject);
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }
    file.write(doc.toJson());
    file.close();
    return true;
}

bool ProjectSerializer::loadProject(const QString& fileName, ProjectData& data) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray fileData = file.readAll();
    file.close();

    QJsonDocument doc(QJsonDocument::fromJson(fileData));
    if (doc.isNull() || !doc.isObject()) {
        return false;
    }

    QJsonObject rootObject = doc.object();

    // Загрузка треугольников
    data.triangles.clear();
    QJsonArray trianglesArray = rootObject["triangles"].toArray();
    for (const QJsonValue& triValue : trianglesArray) {
        QJsonObject triObject = triValue.toObject();

        QJsonArray v1Array = triObject["v1"].toArray();
        QJsonArray v2Array = triObject["v2"].toArray();
        QJsonArray v3Array = triObject["v3"].toArray();

        if (v1Array.size() != 3 || v2Array.size() != 3 || v3Array.size() != 3) {
            continue; // Пропустить некорректные данные
        }

        QSharedPointer<rVect> v1 = QSharedPointer<rVect>::create(
            v1Array[0].toDouble(), v1Array[1].toDouble(), v1Array[2].toDouble());
        QSharedPointer<rVect> v2 = QSharedPointer<rVect>::create(
            v2Array[0].toDouble(), v2Array[1].toDouble(), v2Array[2].toDouble());
        QSharedPointer<rVect> v3 = QSharedPointer<rVect>::create(
            v3Array[0].toDouble(), v3Array[1].toDouble(), v3Array[2].toDouble());

        bool visible = triObject["visible"].toBool(true);

        // Создание узлов из векторов
        QSharedPointer<node> node1 = QSharedPointer<node>::create(v1->getX(), v1->getY(), v1->getZ());
        QSharedPointer<node> node2 = QSharedPointer<node>::create(v2->getX(), v2->getY(), v2->getZ());
        QSharedPointer<node> node3 = QSharedPointer<node>::create(v3->getX(), v3->getY(), v3->getZ());

        // Создание треугольника с параметром 'visible'
        QSharedPointer<triangle> tri = QSharedPointer<triangle>::create(visible, node1, node2, node3);

        data.triangles.append(tri);
    }

    // Загрузка позиции объекта
    QJsonObject positionObject = rootObject["objectPosition"].toObject();
    data.objectPosition.setX(positionObject["x"].toDouble());
    data.objectPosition.setY(positionObject["y"].toDouble());
    data.objectPosition.setZ(positionObject["z"].toDouble());

    // Загрузка поворота
    QJsonObject rotationObject = rootObject["rotation"].toObject();
    data.rotationX = rotationObject["x"].toDouble();
    data.rotationY = rotationObject["y"].toDouble();
    data.rotationZ = rotationObject["z"].toDouble();

    // Загрузка коэффициентов масштабирования
    QJsonObject scalingObject = rootObject["scaling"].toObject();
    data.scalingCoefficients.setX(scalingObject["x"].toDouble());
    data.scalingCoefficients.setY(scalingObject["y"].toDouble());
    data.scalingCoefficients.setZ(scalingObject["z"].toDouble());

    // Загрузка результатов расчётов
    data.absEout.clear();
    QJsonArray absEoutArray = rootObject["absEout"].toArray();
    for (const QJsonValue& value : absEoutArray) {
        data.absEout.append(value.toDouble());
    }

    data.normEout.clear();
    QJsonArray normEoutArray = rootObject["normEout"].toArray();
    for (const QJsonValue& value : normEoutArray) {
        data.normEout.append(value.toDouble());
    }

    data.absEout2D.clear();
    QJsonArray absEout2DArray = rootObject["absEout2D"].toArray();
    for (const QJsonValue& rowValue : absEout2DArray) {
        QJsonArray rowArray = rowValue.toArray();
        QVector<double> row;
        for (const QJsonValue& value : rowArray) {
            row.append(value.toDouble());
        }
        data.absEout2D.append(row);
    }

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

    // Загрузка сохранённых результатов
    data.storedResults = rootObject["storedResults"].toString();

    return true;
}
