#include "modelcontroller.h"
#include <QDebug>
#include <QFileInfo>

ModelController::ModelController(QObject *parent)
    : QObject(parent),
    m_parser(new Parser()),
    m_rayTracer(new RayTracer()),
    m_openGLWidget(nullptr),
    m_rotationX(0.0f),
    m_rotationY(0.0f),
    m_rotationZ(0.0f)
{
    // Соединяем сигналы Parser с соответствующими слотами
    connect(m_parser.get(), &Parser::fileParsed, this,
            [this](const QVector<QVector3D> &vertices, const QVector<QVector<int>> &triangleIndices, const QVector<QSharedPointer<triangle>> &triangles) {
                m_vertices = vertices;
                m_triangleIndices = triangleIndices;
                m_triangles = triangles;

                if (m_openGLWidget) {
                    // Обновление положения камеры для определения видимости
                    rVect observerPosition = m_openGLWidget->getCameraPositionAsRVect();
                    m_rayTracer->determineVisibility(m_triangles, observerPosition);

                    // Установка геометрии в OpenGLWidget
                    m_openGLWidget->setGeometry(m_vertices, m_triangleIndices, m_triangles);
                }
            });

    connect(m_parser.get(), &Parser::modelInfoUpdated, this,
            [this](int nodesCount, int trianglesCount, const QString &fileName) {
                notifyObserversModelLoaded(fileName, nodesCount, trianglesCount);
            });

    connect(m_parser.get(), &Parser::errorOccurred, this,
            [this](const QString &error) {
                emit logMessage("Ошибка: " + error);
            });
}

ModelController::~ModelController() {
    // Очищаем список наблюдателей
    m_observers.clear();

    // Явное освобождение ресурсов
    m_triangles.clear();
    m_vertices.clear();
    m_triangleIndices.clear();
}

bool ModelController::loadModel(const QString &fileName) {
    if (fileName.isEmpty()) {
        emit logMessage("Ошибка: не указан файл для загрузки.");
        return false;
    }

    // Очищаем существующие данные
    m_openGLWidget->clearScene();
    m_parser->clearData();

    // Загружаем новую модель
    emit logMessage("Начата загрузка файла: " + fileName);
    m_parser->readFromObjFile(fileName);

    // Проверяем, успешно ли загружена модель
    if (m_triangles.isEmpty()) {
        emit logMessage("Ошибка: файл не содержит корректных данных объекта.");
        emit notificationRequested("Ошибка загрузки файла", 3); // 3 = Notification::Error
        return false;
    }

    emit logMessage("Файл успешно загружен.");
    emit notificationRequested("Файл успешно загружен", 1); // 1 = Notification::Success
    notifyObserversModelModified();
    return true;
}

void ModelController::closeModel() {
    if (m_openGLWidget) {
        m_openGLWidget->clearScene();
    }

    m_parser->clearData();
    m_triangles.clear();
    m_vertices.clear();
    m_triangleIndices.clear();

    emit logMessage("3D модель закрыта");
    emit notificationRequested("3D модель закрыта", 0); // 0 = Notification::Info

    notifyObserversModelClosed();
}

bool ModelController::hasModel() const {
    return !m_triangles.isEmpty();
}

void ModelController::setOpenGLWidget(OpenGLWidget *widget) {
    if (!widget) {
        emit logMessage("Ошибка: попытка установить nullptr для OpenGLWidget");
        return;
    }

    m_openGLWidget = widget;

    // Если есть данные модели, обновляем виджет
    if (!m_triangles.isEmpty() && m_openGLWidget) {
        m_openGLWidget->setGeometry(m_vertices, m_triangleIndices, m_triangles);
    }
}

void ModelController::setRotation(float x, float y, float z) {
    m_rotationX = x;
    m_rotationY = y;
    m_rotationZ = z;

    if (m_openGLWidget) {
        m_openGLWidget->setRotation(x, y, z);
    }

    emit rotationChanged(x, y, z);

    // Проверяем, что список наблюдателей не пуст
    if (!m_observers.isEmpty()) {
        notifyObserversModelModified();
    }
}

void ModelController::resetRotation() {
    setRotation(0.0f, 0.0f, 0.0f);
}

void ModelController::getRotation(float &x, float &y, float &z) const {
    x = m_rotationX;
    y = m_rotationY;
    z = m_rotationZ;
}

void ModelController::filterMesh() {
    if (m_triangles.isEmpty()) {
        emit logMessage("Ошибка: не загружен 3D объект для фильтрации.");
        emit notificationRequested("Нет загруженного объекта", 2); // 2 = Notification::Warning
        return;
    }

    // Создаем копию треугольников для фильтрации
    QVector<QSharedPointer<triangle>> filteredTriangles = m_triangles;
    MeshFilter::FilterStats stats = m_meshFilter.filterMesh(filteredTriangles);

    // Применяем отфильтрованные треугольники
    if (m_openGLWidget) {
        m_openGLWidget->applyFilteredTriangles(filteredTriangles);
    }

    m_triangles = filteredTriangles;

    emit logMessage(QString("Фильтрация завершена. Всего треугольников: %1, "
                            "Оболочка: %2, Видимые: %3")
                        .arg(stats.totalTriangles)
                        .arg(stats.shellTriangles)
                        .arg(stats.visibleTriangles));
    emit notificationRequested("Фильтрация объекта завершена", 1); // 1 = Notification::Success

    notifyObserversFilteringCompleted(stats);
    notifyObserversModelModified();
}

void ModelController::toggleShadowTriangles() {
    if (m_triangles.isEmpty()) {
        emit logMessage("Ошибка: не загружен 3D объект для обработки.");
        emit notificationRequested("Нет загруженного объекта", 2); // 2 = Notification::Warning
        return;
    }

    int originalCount = getTotalTrianglesCount();

    // Получаем позицию камеры и обрабатываем треугольники
    if (m_openGLWidget) {
        rVect observerPosition = m_openGLWidget->getCameraPositionAsRVect();
        m_openGLWidget->processShadowTriangles(observerPosition);
    }

    int visibleCount = getVisibleTrianglesCount();

    MeshFilter::FilterStats stats;
    stats.totalTriangles = originalCount;
    stats.visibleTriangles = visibleCount;
    stats.removedByShadow = originalCount - visibleCount;

    emit logMessage(QString("Обработка теневых треугольников завершена. Скрыто: %1 треугольников").arg(originalCount - visibleCount));
    emit notificationRequested("Обработка теневых треугольников завершена", 1); // 1 = Notification::Success

    notifyObserversFilteringCompleted(stats);
    notifyObserversModelModified();
}

void ModelController::resetToOriginalModel() {
    if (m_triangles.isEmpty()) {
        emit logMessage("Ошибка: не загружен 3D объект.");
        emit notificationRequested("Нет загруженного объекта", 2); // 2 = Notification::Warning
        return;
    }

    if (m_openGLWidget) {
        m_openGLWidget->setShadowTrianglesFiltering(false);

        int totalTriangles = m_triangles.size();
        MeshFilter::FilterStats stats;
        stats.totalTriangles = totalTriangles;
        stats.shellTriangles = totalTriangles;
        stats.visibleTriangles = totalTriangles;
        stats.removedByShell = 0;
        stats.removedByShadow = 0;

        notifyObserversFilteringCompleted(stats);
    }

    emit logMessage("Отображены все треугольники.");
    emit notificationRequested("Все треугольники отображены", 1); // 1 = Notification::Success

    notifyObserversModelModified();
}

bool ModelController::hasShadowTriangles() const {
    if (m_openGLWidget) {
        return m_openGLWidget->hasShadowTriangles();
    }
    return false;
}

int ModelController::getTotalTrianglesCount() const {
    if (m_openGLWidget) {
        return m_openGLWidget->getTotalTrianglesCount();
    }
    return m_triangles.size();
}

int ModelController::getVisibleTrianglesCount() const {
    if (m_openGLWidget) {
        return m_openGLWidget->getVisibleTrianglesCount();
    }

    // Если нет OpenGLWidget, считаем вручную
    int count = 0;
    for (const auto& tri : m_triangles) {
        if (tri->getVisible()) count++;
    }
    return count;
}

const QVector<QSharedPointer<triangle>>& ModelController::getTriangles() const {
    return m_triangles;
}

rVect ModelController::getDirectionVector() const {
    if (m_openGLWidget) {
        return m_openGLWidget->getDirectionVector();
    }

    // Базовое направление, если нет OpenGLWidget
    return rVect(0, 0, -1);
}

rVect ModelController::getCameraPositionAsRVect() const {
    if (m_openGLWidget) {
        return m_openGLWidget->getCameraPositionAsRVect();
    }

    // Базовая позиция камеры, если нет OpenGLWidget
    return rVect(0, 0, -10);
}

void ModelController::registerObserver(IModelObserver* observer) {
    if (observer && !m_observers.contains(observer)) {
        m_observers.append(observer);
    }
}

void ModelController::unregisterObserver(IModelObserver* observer) {
    m_observers.removeAll(observer);
}

void ModelController::notifyObserversModelLoaded(const QString &fileName, int nodesCount, int trianglesCount) {
    for (auto observer : m_observers) {
        observer->onModelLoaded(fileName, nodesCount, trianglesCount);
    }
}

void ModelController::notifyObserversModelModified() {
    for (auto observer : m_observers) {
        observer->onModelModified();
    }
}

void ModelController::notifyObserversModelClosed() {
    for (auto observer : m_observers) {
        observer->onModelClosed();
    }
}

void ModelController::notifyObserversFilteringCompleted(const MeshFilter::FilterStats &stats) {
    for (auto observer : m_observers) {
        observer->onFilteringCompleted(stats);
    }
}
