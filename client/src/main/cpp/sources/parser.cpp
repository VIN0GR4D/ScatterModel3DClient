#include "parser.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>
#include <QStringList>
#include "Node.h"
#include "Triangle.h"

Parser::Parser(QObject *parent)
    : QObject(parent) {
}

// Функция для чтения данных из OBJ файла
void Parser::readFromObjFile(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Cannot open the file:" << filename;
        emit errorOccurred("Cannot open the file");
        return;
    }
    #ifdef DEBUG
    qDebug() << "File opened successfully: " << filename;
    #endif

    QTextStream in(&file);
    QString line;
    while (!in.atEnd()) {
        line = in.readLine().simplified();
        // Пропускаем пустые строки и комментарии
        if (line.isEmpty() || line.startsWith("#")) continue;
        QStringList parts = line.split(' ');
        processLine(parts);
    }

    file.close();

    qDebug() << "Total nodes read:" << nodes.count();
    qDebug() << "Total triangles read:" << triangles.count();
    qDebug() << "File reading completed: " << filename;

    // Подготовка данных для дальнейшего использования
    QVector<QVector3D> vertices;
    for (const auto& node : nodes) {
        vertices.append(QVector3D(node->getX(), node->getY(), node->getZ()));
    }

    QVector<QVector<int>> triangleIndices;
    for (const auto& triangle : triangles) {
        QVector<int> indices;

        // Получаем индексы вершин треугольника из общего списка узлов
        int indexV1 = nodeIndexCache[triangle->getV1()];
        int indexV2 = nodeIndexCache[triangle->getV2()];
        int indexV3 = nodeIndexCache[triangle->getV3()];

        // Добавляем индексы в список
        indices << indexV1 << indexV2 << indexV3;
        triangleIndices.append(indices);
    }

    // Генерация сигнала о завершении разбора файла с передачей данных
    emit fileParsed(vertices, triangleIndices, triangles);
}

// Обработка строки из файла
void Parser::processLine(const QStringList& parts) {
    if (parts.isEmpty()) return;

    QString type = parts[0].toLower();

    // Обработка разных типов данных
    if (type == "v" && parts.size() >= 4) {
        createNode(parts);
    } else if (type == "f" && parts.size() >= 4) {
        createTriangle(parts);
    } else if (type == "vt" && parts.size() >= 3) {
        createTextureCoordinate(parts);
    } else if (type == "vn" && parts.size() >= 4) {
        createNormal(parts);
    } else if (type == "vp" && parts.size() >= 2) {
        createParameterSpaceVertex(parts);
    } else if (type == "g" && parts.size() >= 2) {
        currentGroupName = parts[1];
        qDebug() << "Group name encountered: " << currentGroupName;
    } else if (type == "o" && parts.size() >= 2) {
        currentObjectName = parts[1];
        qDebug() << "Object name encountered: " << currentObjectName;
    } else {
        qWarning() << "Unknown or invalid line: " << parts.join(" ");
    }
}

// Извлечение числовых значений из строки
QVector<double> Parser::extractNumericValues(const QStringList& parts) {
    QVector<double> values;
    values.reserve(parts.size() - 1); // Резервируем память заранее

    bool ok;
    double value;

    for (int i = 1; i < parts.size(); ++i) {
        value = parts[i].toDouble(&ok);
        if (!ok) {
            qWarning() << "Invalid numeric data: " << parts.join(" ");
            return QVector<double>(); // Возвращаем пустой вектор в случае ошибки
        }
        values.append(value);
    }

    return values;
}

// Создание узла
void Parser::createNode(const QStringList& parts) {
    QVector<double> values = extractNumericValues(parts);
    if (values.size() != 3) {
        qWarning() << "Invalid node data, expected 3 values, got " << values.size();
        return;
    }

    QSharedPointer<node> newNode = QSharedPointer<node>(new node(values[0], values[1], values[2]));
    nodes.append(newNode);
    nodeIndexCache[newNode] = nodes.size() - 1;
}

// Создание треугольника
void Parser::createTriangle(const QStringList& parts) {
    QVector<int> vertexIndices;

    for (int i = 1; i < parts.size(); ++i) {
        QStringList indices = parts[i].split('/');
        bool okV;

        QSharedPointer<node> vNode = nodes[indices[0].toInt(&okV) - 1];
        if (!okV || !nodeIndexCache.contains(vNode)) {
            qWarning() << "Invalid vertex index for triangle: " << parts[i];
            return;
        }

        int vIndex = nodeIndexCache[vNode];  // Использование кэша для получения индекса
        vertexIndices.append(vIndex);
    }

    if (vertexIndices.size() >= 3) {
        auto V1 = nodes.at(vertexIndices[0]);
        auto V2 = nodes.at(vertexIndices[1]);
        auto V3 = nodes.at(vertexIndices[2]);
        auto newTriangle = QSharedPointer<triangle>::create(true, V1, V2, V3);
        triangles.append(newTriangle);
    }
}

// Создание текстурной координаты
void Parser::createTextureCoordinate(const QStringList& parts) {
    QVector<double> values = extractNumericValues(parts);
    // Для текстурных координат ожидаем от двух до трех значений: u, v, [w]
    if (values.isEmpty() || values.size() < 2 || values.size() > 3) {
        qWarning() << "Invalid texture coordinate data: " << parts.join(" ");
        return;
    }

    double u = values[0];
    double v = values[1];
    double w = (values.size() == 3) ? values[2] : 0.0; // w is optional

    textureCoordinates.append(QVector3D(u, v, w));
}

void Parser::createNormal(const QStringList& parts) {
    QVector<double> values = extractNumericValues(parts);
    // Для нормалей ожидаем три значения: x, y, z
    if (values.size() != 3) {
        qWarning() << "Invalid normal data: " << parts.join(" ");
        return;
    }

    normals.append(QVector3D(values[0], values[1], values[2]));
}

// Создание вершины в пространстве параметров
void Parser::createParameterSpaceVertex(const QStringList& parts) {
    QVector<double> values = extractNumericValues(parts);
    // Для вершин в пространстве параметров ожидаем от одного до трех значений: u, v, w (v и w опциональны)
    if (values.isEmpty() || values.size() > 3) {
        qWarning() << "Invalid parameter space vertex data: " << parts.join(" ");
        return;
    }

    double u = values[0];
    double v = values.size() > 1 ? values[1] : 0.0;  // v is optional, default to 0.0 if not present
    double w = values.size() > 2 ? values[2] : 0.0;  // w is optional, default to 0.0 if not present

    parameterSpaceVertices.append(QVector3D(u, v, w));
}

// Очистка данных парсера
void Parser::clearData() {
    nodes.clear();
    triangles.clear();
    // Очистка всех связанных данных
    nodeIndexCache.clear();
    textureCoordinates.clear();
    normals.clear();
    parameterSpaceVertices.clear();
    qDebug() << "Parser data cleared.";
}
