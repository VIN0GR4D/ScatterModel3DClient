#pragma once

#include <QString>
#include <QJsonObject>
#include <QVector3D>
#include <QSharedPointer>
#include <QVector>
#include "triangle.h"

class ProjectData {
public:
    QVector<QSharedPointer<triangle>> triangles;
    QVector3D objectPosition;
    float rotationX;
    float rotationY;
    float rotationZ;
    QVector3D scalingCoefficients;
    QVector<double> absEout;
    QVector<double> normEout;
    QVector<QVector<double>> absEout2D;
    QVector<QVector<double>> normEout2D;
    QString storedResults;
    QVector<QSharedPointer<triangle>> shellTriangles;
    QVector<QSharedPointer<triangle>> shadowShellTriangles;

    QString projectName;
    QString workingDirectory;
};

class ProjectSerializer {
public:
    static bool saveProject(const QString& fileName, const ProjectData& data);
    static bool loadProject(const QString& fileName, ProjectData& data);
};
