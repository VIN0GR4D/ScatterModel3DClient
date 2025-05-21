#ifndef MODELCONTROLLER_H
#define MODELCONTROLLER_H

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#undef byte

// Стандартные заголовки C++
#include <cstddef>
#include <memory>
#include <vector>

// Заголовки Qt
#include <QObject>
#include <QVector>
#include <QVector3D>
#include <QSharedPointer>

// Ваши собственные заголовки
#include "Triangle.h"
#include "Node.h"
#include "parser.h"
#include "raytracer.h"
#include "meshfilter.h"
#include "openglwidget.h"

// Предварительное объявление для избежания циклической зависимости
class OpenGLWidget;

// Интерфейс для наблюдателей за изменениями модели
class IModelObserver {
public:
    virtual ~IModelObserver() = default;
    virtual void onModelLoaded(const QString &fileName, int nodesCount, int trianglesCount) = 0;
    virtual void onModelModified() = 0;
    virtual void onModelClosed() = 0;
    virtual void onFilteringCompleted(const MeshFilter::FilterStats &stats) = 0;
};

class ModelController : public QObject
{
    Q_OBJECT

public:
    explicit ModelController(QObject *parent = nullptr);
    ~ModelController();

    // Методы для управления моделью
    bool loadModel(const QString &fileName);
    void closeModel();
    bool hasModel() const;

    // Установка визуального компонента
    void setOpenGLWidget(OpenGLWidget *widget);

    // Методы для управления трансформациями
    void setRotation(float x, float y, float z);
    void resetRotation();
    void getRotation(float &x, float &y, float &z) const;
    void resetScale();

    // Методы для управления фильтрацией
    void filterMesh();
    void toggleShadowTriangles();
    void resetToOriginalModel();
    bool hasShadowTriangles() const;

    // Методы для получения информации о модели
    int getTotalTrianglesCount() const;
    int getVisibleTrianglesCount() const;
    const QVector<QSharedPointer<triangle>>& getTriangles() const;
    rVect getDirectionVector() const;
    rVect getCameraPositionAsRVect() const;

    // Методы для работы с наблюдателями
    void registerObserver(IModelObserver* observer);
    void unregisterObserver(IModelObserver* observer);

signals:
    void logMessage(const QString &message);
    void notificationRequested(const QString &message, int type);
    void rotationChanged(float x, float y, float z);

private:
    // Внутренние компоненты
    std::unique_ptr<Parser> m_parser;
    std::unique_ptr<RayTracer> m_rayTracer;
    MeshFilter m_meshFilter;
    OpenGLWidget *m_openGLWidget;

    // Исходный набор треугольников
    QVector<QSharedPointer<triangle>> m_originalTriangles;

    // Флаг, указывающий, было ли применено фильтрование оболочки
    bool m_shellFilteringApplied = false;

    // Флаг, указывающий, было ли применено фильтрование теней
    bool m_shadowFilteringApplied = false;

    // Данные модели
    QVector<QSharedPointer<triangle>> m_triangles;
    QVector<QSharedPointer<node>> m_nodes;
    QVector<QVector3D> m_vertices;
    QVector<QVector<int>> m_triangleIndices;

    // Параметры трансформации
    float m_rotationX;
    float m_rotationY;
    float m_rotationZ;

    // Список наблюдателей
    QVector<IModelObserver*> m_observers;

    // Вспомогательные методы
    void notifyObserversModelLoaded(const QString &fileName, int nodesCount, int trianglesCount);
    void notifyObserversModelModified();
    void notifyObserversModelClosed();
    void notifyObserversFilteringCompleted(const MeshFilter::FilterStats &stats);

    MeshFilter::FilterStats m_currentStats; // Текущая статистика фильтрации
};

#endif // MODELCONTROLLER_H
