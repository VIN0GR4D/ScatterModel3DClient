#include "openglwidget.h"
#include <QMatrix4x4>

OpenGLWidget::OpenGLWidget(QWidget *parent)
    : QOpenGLWidget(parent), rotationX(0.0f), rotationY(0.0f), rotationZ(0.0f), scale(1.0f), objectPosition(0.0f, 0.0f, 0.0f), gridVisible(true) {
    QSurfaceFormat format;
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setVersion(3, 0);
    setFormat(format);

    setFocusPolicy(Qt::StrongFocus);
    // colorTable = {Qt::red, Qt::green, Qt::blue, Qt::yellow, Qt::magenta, Qt::cyan, Qt::white, Qt::gray};
    colorTable = {Qt::white};
}

// Установка геометрии (вершин и треугольников)
void OpenGLWidget::setGeometry(const QVector<QVector3D> &v, const QVector<QVector<int>> &t, const QVector<QSharedPointer<triangle>> &tri) {
    vertices = v;
    triangleIndices = t;
    triangles = tri;

    computeBoundingVolume(); // Вычисление ограничивающего объема
    triangleColors.clear();
    for (int i = 0; i < triangleIndices.size(); ++i) {
        triangleColors.append(chooseColorForTriangle(i)); // Назначение цвета для каждого треугольника
    }

    resetCamera(); // Сброс параметров камеры
    geometryChanged = true;
    update(); // Обновление виджета для перерисовки
}

// Вычисление оптимального коэффициента отдаления камеры
float OpenGLWidget::calculateOptimalZoomOutFactor(float boundingSphereRadius) {
    float baseDistance = 10.0f + boundingSphereRadius;

    float minDistance = 5.0f;
    float maxDistance = 2000.0f;

    float adjustedDistance = qBound(minDistance, baseDistance, maxDistance);

    return adjustedDistance / boundingSphereRadius;
}

const QVector<QSharedPointer<triangle>>& OpenGLWidget::getTriangles() const {
    return triangles;
}

const QVector<QVector3D>& OpenGLWidget::getVertices() const {
    return vertices;
}

const QVector<QVector<int>>& OpenGLWidget::getIndices() const {
    return triangleIndices;
}

// Вычисление ограничивающего объема для геометрии
void OpenGLWidget::computeBoundingVolume() {
    if (vertices.isEmpty()) return;

    QVector3D min = vertices[0], max = vertices[0];
    float maxRadiusSq = 0.0f;

    QVector3D center = vertices[0];  // Начальное приближение центра
    for (const auto& vertex : vertices) {
        min.setX(qMin(min.x(), vertex.x()));
        min.setY(qMin(min.y(), vertex.y()));
        min.setZ(qMin(min.z(), vertex.z()));
        max.setX(qMax(max.x(), vertex.x()));
        max.setY(qMax(max.y(), vertex.y()));
        max.setZ(qMax(max.z(), vertex.z()));
        center += vertex;
    }
    center /= vertices.size();

    for (const auto& vertex : vertices) {
        float currentRadiusSq = (vertex - center).lengthSquared();
        maxRadiusSq = qMax(maxRadiusSq, currentRadiusSq);
    }

    updateCameraPosition(center, std::sqrt(maxRadiusSq)); // Обновление позиции камеры
}

// Получение текущей позиции камеры
QVector3D OpenGLWidget::getCameraPosition() const {
    return cameraPosition;
}

// Обновление позиции камеры
void OpenGLWidget::updateCameraPosition(const QVector3D& center, float radius) {
    const float fieldOfView = 45.0f;
    float viewAngleRadians = qDegreesToRadians(fieldOfView / 2.0f);
    float distance = radius / sin(viewAngleRadians) * 2.0f;
    cameraPosition = QVector3D(center.x(), center.y(), center.z() - distance);

    // Обновление позиции источника света, прикрепленного к камере
    lightPosition = cameraPosition + QVector3D(0.0f, 0.0f, 10.0f);

    // qDebug() << "Camera position updated:" << cameraPosition;
    update();
}

// Сброс положения камеры
void OpenGLWidget::resetCamera() {
    rotationX = 0.0f;
    rotationY = 0.0f;
    rotationZ = 0.0f;
    scale = 1.0f;
}

// Выбор цвета для треугольника на основе его индекса
QColor OpenGLWidget::chooseColorForTriangle(int triangleIndex) {
    return colorTable[triangleIndex % colorTable.size()];
}

// Обновление видимости треугольников
void OpenGLWidget::updateVisibility(const QVector<QSharedPointer<triangle>>& triangles) {
    rayTracer.setTriangles(triangles); // Установка треугольников для трассировщика лучей
    rVect observerPosition = QVector3DToRVect(cameraPosition); // Преобразование QVector3D в rVect
    rayTracer.determineVisibility(triangles, observerPosition); // Определение видимости треугольников относительно позиции камеры
}

void OpenGLWidget::setRotation(float x, float y, float z) {
    rotationX = x;
    rotationY = y;
    rotationZ = z;
    update();
}

// Инициализация OpenGL
void OpenGLWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.0, 0.0, 0.0, 1.0); // Черный фон
    glEnable(GL_DEPTH_TEST); // Включить тест глубины
    glEnable(GL_LIGHTING); // Включить освещение
    glEnable(GL_LIGHT0); // Включить источник света 0
    glEnable(GL_COLOR_MATERIAL); // Включить цвет материала

    // Настройка источника света 0
    GLfloat light0Ambient[] = { 0.1f, 0.1f, 0.1f, 1.0f };
    GLfloat light0Diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat light0Specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, light0Ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light0Diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light0Specular);

    // Настройка второго источника света (опционально)
    glEnable(GL_LIGHT1);
    GLfloat light1Position[] = { 0.0f, 1.0f, 0.0f, 0.0f };
    GLfloat light1Ambient[] = { 0.0f, 0.0f, 0.0f, 1.0f };
    GLfloat light1Diffuse[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    GLfloat light1Specular[] = { 0.5f, 0.5f, 0.5f, 1.0f };

    glLightfv(GL_LIGHT1, GL_POSITION, light1Position);
    glLightfv(GL_LIGHT1, GL_AMBIENT, light1Ambient);
    glLightfv(GL_LIGHT1, GL_DIFFUSE, light1Diffuse);
    glLightfv(GL_LIGHT1, GL_SPECULAR, light1Specular);

    // Настройка материала
    GLfloat matAmbient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat matDiffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat matSpecular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat matShininess[] = { 50.0f };

    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, matAmbient);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, matDiffuse);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, matSpecular);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SHININESS, matShininess);

    qDebug() << "OpenGL initialized with multiple lights.";
}

// Изменение размера окна OpenGL
void OpenGLWidget::resizeGL(int w, int h) {
    if (currentWidth == w && currentHeight == h) return;
    currentWidth = w;
    currentHeight = h;

    glViewport(0, 0, w, h);
    projection.setToIdentity();
    projection.perspective(45.0f, GLfloat(w) / h, 0.1f, 5000.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection.constData());
    glMatrixMode(GL_MODELVIEW);

    calculatePixelOccupation();
}

// Отрисовка сцены
void OpenGLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Применение трансформаций камеры
    glTranslatef(-cameraPosition.x(), -cameraPosition.y(), cameraPosition.z());

    // Применение трансформаций объекта
    glTranslatef(objectPosition.x(), objectPosition.y(), objectPosition.z());
    glScalef(scale, scale, scale);
    glRotatef(rotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotationY, 0.0f, 1.0f, 0.0f);
    glRotatef(rotationZ, 0.0f, 0.0f, 1.0f);

    // Установка позиции источника света, прикрепленного к камере
    GLfloat lightPos[] = { lightPosition.x(), lightPosition.y(), lightPosition.z(), 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // Отрисовка сетки
    drawGrid();

    // Обновление видимости только при изменении параметров камеры или загрузке новых данных
    if (geometryChanged) {
        updateVisibility(triangles);
        geometryChanged = false; // Сбросить флаг изменения геометрии
    }

    // Отрисовка видимых треугольников
    for (int i = 0; i < triangles.size(); ++i) {
        auto triangle = triangles[i];
        if (!triangle->getVisible()) continue; // Пропускаем невидимые треугольники

        QColor color = triangleColors[i];
        glColor3f(color.redF(), color.greenF(), color.blueF());

        glBegin(GL_TRIANGLES);
        auto v1 = triangles[i]->getV1();
        auto v2 = triangles[i]->getV2();
        auto v3 = triangles[i]->getV3();

        rVect normal = rayTracer.computeNormal(triangle);
        QVector3D normalVec(normal.getX(), normal.getY(), normal.getZ());
        glNormal3f(normalVec.x(), normalVec.y(), normalVec.z());

        glVertex3f(v1->getX(), v1->getY(), v1->getZ());
        glVertex3f(v2->getX(), v2->getY(), v2->getZ());
        glVertex3f(v3->getX(), v3->getY(), v3->getZ());
        glEnd();
    }

    calculatePixelOccupation();

    // Отрисовка индикатора координат
    drawCoordinateIndicator();
}

// Очистка сцены
void OpenGLWidget::clearScene() {
    qDebug() << "Clearing scene...";
    vertices.clear();
    triangleIndices.clear();
    triangles.clear();
    triangleColors.clear();

    geometryChanged = true; // Установка флага, что геометрия изменилась
    update(); // Вызов перерисовки виджета
}

// Преобразование QVector3D в rVect
rVect OpenGLWidget::QVector3DToRVect(const QVector3D& vec) {
    return rVect(vec.x(), vec.y(), vec.z());
}

// Обработка событий нажатия кнопки мыши
void OpenGLWidget::mousePressEvent(QMouseEvent *event) {
    lastMousePosition = event->position().toPoint();
}

// Обработка событий перемещения мыши
void OpenGLWidget::mouseMoveEvent(QMouseEvent *event) {
    QPointF pos = event->position();
    float dx = pos.x() - lastMousePosition.x();
    float dy = pos.y() - lastMousePosition.y();

    if (dx == 0 && dy == 0) return;

    const float sensitivity = 0.5f; // Регулировка чувствительности вращения
    if (event->buttons() & Qt::LeftButton) {
        rotationX += dy * sensitivity;
        rotationY += dx * sensitivity;
    } else if (event->buttons() & Qt::RightButton) {
        rotationZ += dx * sensitivity;
    }

    emit rotationChanged(rotationX, rotationY, rotationZ); // Испускаем сигнал для обновления угла поворота в интерфейсе

    update();
    lastMousePosition = pos.toPoint(); // Обновление последней позиции мыши
}

// Обработка событий колеса мыши (масштабирование)
void OpenGLWidget::wheelEvent(QWheelEvent *event) {
    const float zoomSensitivity = 0.05f; // Чувствительность масштабирования
    int delta = event->angleDelta().y();
    float factor = delta > 0 ? (1 + zoomSensitivity) : (1 - zoomSensitivity);
    scale *= factor;
    update();
}

// Функции для подсчёта пиксилей в изображении объекта
void OpenGLWidget::calculatePixelOccupation() {
    // Получение текущих ModelView и Projection матриц
    QMatrix4x4 modelViewMatrix;
    QMatrix4x4 projectionMatrix;
    glGetFloatv(GL_MODELVIEW_MATRIX, modelViewMatrix.data());
    glGetFloatv(GL_PROJECTION_MATRIX, projectionMatrix.data());

    // qDebug() << "ModelView Matrix:" << modelViewMatrix;
    // qDebug() << "Projection Matrix:" << projectionMatrix;

    int windowWidth = this->width();
    int windowHeight = this->height();

    QVector<QVector3D> screenCoords;
    for (const QVector3D& vertex : vertices) {
        QVector3D screenCoord = projectToScreen(vertex, modelViewMatrix, projectionMatrix, windowWidth, windowHeight);
        screenCoords.append(screenCoord);
        // qDebug() << "World Coord:" << vertex << "Screen Coord:" << screenCoord;
    }

    // Найти минимальные и максимальные координаты
    float minX = windowWidth, minY = windowHeight, maxX = 0, maxY = 0;
    for (const QVector3D& coord : screenCoords) {
        minX = qMin(minX, coord.x());
        minY = qMin(minY, coord.y());
        maxX = qMax(maxX, coord.x());
        maxY = qMax(maxY, coord.y());
    }

    // // Площадь в пикселях
    // int pixelWidth = maxX - minX;
    // int pixelHeight = maxY - minY;
    // int pixelArea = pixelWidth * pixelHeight;

    // qDebug() << "Object occupies" << pixelArea << "pixels (" << pixelWidth << "x" << pixelHeight << ")";
}

QVector3D OpenGLWidget::projectToScreen(const QVector3D& worldCoord, const QMatrix4x4& modelView, const QMatrix4x4& projection, int windowWidth, int windowHeight) {
    QVector4D clipSpaceCoord = projection * modelView * QVector4D(worldCoord, 1.0);
    if (clipSpaceCoord.w() != 0.0) {
        clipSpaceCoord /= clipSpaceCoord.w();
    }
    QVector3D ndcCoord = clipSpaceCoord.toVector3D();

    float x = (ndcCoord.x() + 1.0) * 0.5 * windowWidth;
    float y = (1.0 - ndcCoord.y()) * 0.5 * windowHeight;
    return QVector3D(x, y, ndcCoord.z());
}

void OpenGLWidget::drawCoordinateIndicator() {
    // Сохраняем текущие матрицы
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    // Устанавливаем орфографическую проекцию
    int width = this->width();
    int height = this->height();
    glOrtho(0, width, 0, height, -1, 1);

    // Сбрасываем модельно-видовую матрицу
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Отключаем глубинный тест и освещение для рисования индикатора поверх сцены
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    // Определяем размер индикатора
    float indicatorSize = 50.0f; // Размер индикатора в пикселях

    // Устанавливаем позицию индикатора в левом нижнем углу
    glTranslatef(10 + indicatorSize / 2, 10 + indicatorSize / 2, 0);

    // Применяем текущие вращения к индикатору
    glRotatef(rotationX, 1.0f, 0.0f, 0.0f);
    glRotatef(rotationY, 0.0f, 1.0f, 0.0f);
    glRotatef(rotationZ, 0.0f, 0.0f, 1.0f);

    // Рисуем оси координат
    glBegin(GL_LINES);
    // Ось X - красная
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(0, 0, 0);
    glVertex3f(indicatorSize, 0, 0);

    // Ось Y - зеленая
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(0, 0, 0);
    glVertex3f(0, indicatorSize, 0);

    // Ось Z - синяя
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, indicatorSize);
    glEnd();

    // Восстанавливаем состояние OpenGL
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void OpenGLWidget::drawGrid() {
    if (!gridVisible) return;

    // Сохраняем текущее состояние матриц и атрибутов
    glPushMatrix();
    glPushAttrib(GL_ENABLE_BIT);

    // Отключаем освещение и глубинный тест для рисования сетки
    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    // Устанавливаем цвет сетки (например, серый)
    glColor3f(0.5f, 0.5f, 0.5f);

    // Устанавливаем размер сетки и шаг
    float gridSize = 1000.0f; // Размер сетки
    float step = 10.0f;       // Шаг между линиями

    // Рисуем сетку на плоскости XZ
    glBegin(GL_LINES);
    for (float i = -gridSize; i <= gridSize; i += step) {
        // Линии параллельные оси Z
        glVertex3f(i, 0.0f, -gridSize);
        glVertex3f(i, 0.0f, gridSize);

        // Линии параллельные оси X
        glVertex3f(-gridSize, 0.0f, i);
        glVertex3f(gridSize, 0.0f, i);
    }
    glEnd();

    // Восстанавливаем состояние матриц и атрибутов
    glPopAttrib();
    glPopMatrix();
}

void OpenGLWidget::setGridVisible(bool visible) {
    gridVisible = visible;
    update(); // Запрос перерисовки
}

void OpenGLWidget::setObjectPosition(const QVector3D& position) {
    objectPosition = position;
    update(); // Обновляем виджет для перерисовки
}

QVector3D OpenGLWidget::getObjectPosition() const {
    return objectPosition;
}

void OpenGLWidget::setTriangles(const QVector<QSharedPointer<triangle>>& tri) {
    triangles = tri;
    vertices.clear();
    triangleIndices.clear();

    for (const auto& triangle : triangles) {
        int index = vertices.size();
        vertices.append(QVector3D(triangle->getV1()->getX(), triangle->getV1()->getY(), triangle->getV1()->getZ()));
        vertices.append(QVector3D(triangle->getV2()->getX(), triangle->getV2()->getY(), triangle->getV2()->getZ()));
        vertices.append(QVector3D(triangle->getV3()->getX(), triangle->getV3()->getY(), triangle->getV3()->getZ()));

        triangleIndices.append({index, index + 1, index + 2});
    }

    computeBoundingVolume();
    triangleColors.clear();
    for (int i = 0; i < triangleIndices.size(); ++i) {
        triangleColors.append(chooseColorForTriangle(i));
    }

    resetCamera();
    geometryChanged = true;
    update();
}

void OpenGLWidget::setScalingCoefficients(const QVector3D& scaling) {
    scale = (scaling.x() + scaling.y() + scaling.z()) / 3.0f; // Пример усреднения
    update();
}
