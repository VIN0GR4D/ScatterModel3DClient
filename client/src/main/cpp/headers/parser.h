#ifndef PARSER_H
#define PARSER_H

#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QStringList>
#include <QVector>
#include <QVector2D>
#include <QVector3D>
#include <QSharedPointer>
#include "Node.h"
#include "Triangle.h"

class Parser : public QObject {
    Q_OBJECT

public:
    explicit Parser(QObject *parent = nullptr);
    void readFromObjFile(const QString& filename);
    void clearData();
    void processLine(const QStringList& parts);

    QVector<QSharedPointer<node>> nodes;
    QVector<QSharedPointer<triangle>> triangles;

signals:
    void errorOccurred(const QString &error);
    void fileParsed(const QVector<QVector3D> &vertices, const QVector<QVector<int>> &triangles, const QVector<QSharedPointer<triangle>> &trianglesObjects);
    void modelInfoUpdated(int nodesCount, int trianglesCount, const QString &fileName); // Информация о количестве вершин, треугольников и названию файла объекта

private:
    struct TriangleIndices {
        int vertexIndex;
        int textureIndex;
        int normalIndex;

        TriangleIndices(int vIdx, int tIdx, int nIdx)
            : vertexIndex(vIdx), textureIndex(tIdx), normalIndex(nIdx) {}
    };

    void createNode(const QStringList& parts);
    void createTriangle(const QStringList& parts);
    void createTextureCoordinate(const QStringList& parts);
    void createNormal(const QStringList& parts);
    void createParameterSpaceVertex(const QStringList& parts);

    QHash<QSharedPointer<node>, int> nodeIndexCache;
    QVector<QVector3D> textureCoordinates;
    QVector<QVector3D> normals;
    QVector<QVector3D> parameterSpaceVertices;
    QVector<TriangleIndices> faceIndices;
    QVector<double> extractNumericValues(const QStringList& parts);
    QString currentObjectName;
    QString currentGroupName;
};

#endif // PARSER_H
