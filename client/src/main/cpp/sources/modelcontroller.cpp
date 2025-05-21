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
    m_rotationZ(0.0f),
    m_currentStats()
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

    // Сбрасываем статистику фильтрации для нового объекта
    int totalTriangles = m_triangles.size();
    MeshFilter::FilterStats newStats;
    newStats.totalTriangles = totalTriangles;
    newStats.shellTriangles = totalTriangles;  // Изначально все треугольники считаются частью оболочки
    newStats.visibleTriangles = totalTriangles; // Изначально все треугольники видимы
    newStats.removedByShell = 0;
    newStats.removedByShadow = 0;
    newStats.filterType = MeshFilter::ResetFilter;

    m_originalTriangles = m_triangles; // Сохраняем исходную копию
    m_shellFilteringApplied = false; // Сбрасываем флаги
    m_shadowFilteringApplied = false;

    // Обновляем текущую статистику
    m_currentStats = newStats;

    // Уведомляем наблюдателей о завершении загрузки модели и об обновлении статистики
    notifyObserversFilteringCompleted(m_currentStats);

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

    m_originalTriangles.clear();
    m_shellFilteringApplied = false;
    m_shadowFilteringApplied = false;

    // Сбрасываем статистику фильтрации при закрытии модели
    MeshFilter::FilterStats emptyStats;
    m_currentStats = emptyStats;

    // Уведомляем наблюдателей об обновлении статистики
    notifyObserversFilteringCompleted(m_currentStats);

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

    // Используем исходный набор треугольников, если он есть
    QVector<QSharedPointer<triangle>> sourceTriangles;
    if (!m_originalTriangles.isEmpty()) {
        sourceTriangles = m_originalTriangles;
    } else {
        sourceTriangles = m_triangles;
        m_originalTriangles = m_triangles; // Сохраняем для будущего использования
    }

    // Если уже применено фильтрование теней, сохраняем состояние видимости
    QVector<bool> visibilityState;
    if (m_shadowFilteringApplied) {
        for (const auto& tri : m_triangles) {
            visibilityState.append(tri->getVisible());
        }
    }

    // Создаем копию треугольников для фильтрации
    QVector<QSharedPointer<triangle>> filteredTriangles = sourceTriangles;

    // Фильтруем треугольники для определения оболочки
    MeshFilter::FilterStats stats = m_meshFilter.filterMesh(filteredTriangles);

    // Если ранее было применено фильтрование теней, восстанавливаем состояние видимости
    if (m_shadowFilteringApplied && filteredTriangles.size() == visibilityState.size()) {
        for (int i = 0; i < filteredTriangles.size(); ++i) {
            filteredTriangles[i]->setVisible(visibilityState[i]);
        }

        // Корректируем статистику
        stats.visibleTriangles = 0;
        for (const auto& tri : filteredTriangles) {
            if (tri->getVisible()) stats.visibleTriangles++;
        }
        stats.removedByShadow = filteredTriangles.size() - stats.visibleTriangles;
    }

    // Обновляем текущую статистику по оболочке (shell)
    m_currentStats.totalTriangles = stats.totalTriangles;
    m_currentStats.shellTriangles = stats.shellTriangles;
    m_currentStats.removedByShell = stats.removedByShell;

    // Сохраняем текущие данные о видимости, если они уже были
    if (m_shadowFilteringApplied) {
        stats.visibleTriangles = m_currentStats.visibleTriangles;
        stats.removedByShadow = m_currentStats.removedByShadow;
    } else {
        stats.visibleTriangles = stats.shellTriangles; // По умолчанию все треугольники оболочки видимы
        stats.removedByShadow = 0;
    }

    // Устанавливаем тип фильтрации
    stats.filterType = MeshFilter::ShellFilter;

    // Обновляем текущую статистику полностью
    m_currentStats = stats;
    m_shellFilteringApplied = true;

    // Применяем отфильтрованные треугольники, сохраняя трансформацию
    if (m_openGLWidget) {
        // Используем новый метод вместо applyFilteredTriangles
        m_openGLWidget->setTrianglesPreservingTransform(filteredTriangles);
    }

    m_triangles = filteredTriangles;

    emit logMessage(QString("Фильтрация завершена. Всего треугольников: %1, "
                            "Оболочка: %2, Видимые: %3")
                        .arg(stats.totalTriangles)
                        .arg(stats.shellTriangles)
                        .arg(stats.visibleTriangles));
    emit notificationRequested("Фильтрация объекта завершена", 1); // 1 = Notification::Success

    notifyObserversFilteringCompleted(m_currentStats);
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

    // Создаем структуру только для видимости (shadow)
    MeshFilter::FilterStats stats;
    stats.totalTriangles = originalCount;
    stats.visibleTriangles = visibleCount;
    stats.removedByShadow = originalCount - visibleCount;

    // Сохраняем текущие данные об оболочке
    if (m_shellFilteringApplied) {
        stats.shellTriangles = m_currentStats.shellTriangles;
        stats.removedByShell = m_currentStats.removedByShell;
    } else {
        stats.shellTriangles = originalCount; // По умолчанию все треугольники считаются оболочкой
        stats.removedByShell = 0;
    }

    // Устанавливаем тип фильтрации
    stats.filterType = MeshFilter::ShadowFilter;

    // Обновляем текущую статистику полностью
    m_currentStats = stats;
    m_shadowFilteringApplied = true;

    emit logMessage(QString("Обработка теневых треугольников завершена. Скрыто: %1 треугольников").arg(originalCount - visibleCount));
    emit notificationRequested("Обработка теневых треугольников завершена", 1); // 1 = Notification::Success

    notifyObserversFilteringCompleted(m_currentStats);
    notifyObserversModelModified();
}

void ModelController::resetToOriginalModel() {
    if (m_triangles.isEmpty()) {
        emit logMessage("Ошибка: не загружен 3D объект.");
        emit notificationRequested("Нет загруженного объекта", 2); // 2 = Notification::Warning
        return;
    }

    // Восстанавливаем исходный набор треугольников, если он был сохранен
    if (!m_originalTriangles.isEmpty()) {
        m_triangles = m_originalTriangles;
    }

    if (m_openGLWidget) {
        // Устанавливаем все треугольники видимыми
        for (auto& tri : m_triangles) {
            tri->setVisible(true);
        }

        // Используем новый метод для сохранения трансформации
        m_openGLWidget->setTrianglesPreservingTransform(m_triangles);
        m_openGLWidget->setShadowTrianglesFiltering(false);

        int totalTriangles = m_triangles.size();

        // Создаем структуру для сброса всех фильтров
        MeshFilter::FilterStats stats;
        stats.totalTriangles = totalTriangles;
        stats.shellTriangles = totalTriangles; // Все треугольники в оболочке
        stats.visibleTriangles = totalTriangles; // Все треугольники видимы
        stats.removedByShell = 0;
        stats.removedByShadow = 0;
        stats.filterType = MeshFilter::ResetFilter;

        // Обновляем текущую статистику полностью
        m_currentStats = stats;

        // Сбрасываем флаги
        m_shellFilteringApplied = false;
        m_shadowFilteringApplied = false;

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

void ModelController::resetScale() {
    if (m_openGLWidget) {
        m_openGLWidget->resetScale();
    }

    // Уведомляем наблюдателей об изменении модели
    notifyObserversModelModified();
}
