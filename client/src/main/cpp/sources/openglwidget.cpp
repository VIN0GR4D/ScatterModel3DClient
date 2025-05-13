#include "openglwidget.h"
#include "geometryutils.h"
#include <QMatrix4x4>
#include <GL/glu.h>

OpenGLWidget::OpenGLWidget(QWidget *parent)
    : QOpenGLWidget(parent), rotationX(0.0f), rotationY(0.0f), rotationZ(0.0f), scale(1.0f), objectPosition(0.0f, 0.0f, 0.0f), gridVisible(true), showUnderlyingSurface(false), hasLoadedObject(false) {
    QSurfaceFormat format;
    format.setProfile(QSurfaceFormat::CompatibilityProfile);
    format.setVersion(3, 0);
    setFormat(format);

    setFocusPolicy(Qt::StrongFocus);
    // colorTable = {Qt::red, Qt::green, Qt::blue, Qt::yellow, Qt::magenta, Qt::cyan, Qt::white, Qt::gray};
    colorTable = {Qt::white};

    // Инициализируем параметры сетки по умолчанию
    initializeDefaultGrid();
}

// Установка геометрии (вершин и треугольников)
void OpenGLWidget::setGeometry(const QVector<QVector3D> &v, const QVector<QVector<int>> &t, const QVector<QSharedPointer<triangle>> &tri) {
    vertices = v;
    triangleIndices = t;
    triangles = tri;
    hasLoadedObject = true;

    computeBoundingVolume(); // Вычисление ограничивающего объема
    triangleColors.clear();
    for (int i = 0; i < triangleIndices.size(); ++i) {
        triangleColors.append(chooseColorForTriangle(i)); // Назначение цвета для каждого треугольника
    }

    resetCamera(); // Сброс параметров камеры
    gridParametersChanged = true; // Устанавливаем флаг обновления сетки
    geometryChanged = true;
    update(); // Обновление виджета для перерисовки
}

void OpenGLWidget::setShadowTrianglesFiltering(bool enable) {
    filterShadowTriangles = enable;
    update();
}

void OpenGLWidget::updateScene() {
    geometryChanged = true;  // Устанавливаем флаг для перерисовки
    update();  // Запрашиваем перерисовку
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

bool OpenGLWidget::hasShadowTriangles() const {
    int visibleCount = 0;
    for (const auto& tri : triangles) {
        if (tri->getVisible()) visibleCount++;
    }
    return visibleCount < triangles.size();
}

void OpenGLWidget::processShadowTriangles(const rVect& observerPosition) {
    rayTracer.determineVisibility(triangles, observerPosition);
    setShadowTrianglesFiltering(true);
    update();
}

int OpenGLWidget::getTotalTrianglesCount() const {
    return triangles.size();
}

int OpenGLWidget::getVisibleTrianglesCount() const {
    int count = 0;
    for (const auto& tri : triangles) {
        if (tri->getVisible()) count++;
    }
    return count;
}

rVect OpenGLWidget::getDirectionVector() const {
    // Матрица преобразования для учета поворотов модели
    QMatrix4x4 rotationMatrix;
    rotationMatrix.setToIdentity();
    rotationMatrix.rotate(rotationX, 1.0f, 0.0f, 0.0f);
    rotationMatrix.rotate(rotationY, 0.0f, 1.0f, 0.0f);
    rotationMatrix.rotate(rotationZ, 0.0f, 0.0f, 1.0f);

    // Направление по умолчанию (против оси Z в новой системе координат)
    QVector3D defaultDirection(0.0f, 0.0f, -1.0f);
    QVector3D transformedDirection = rotationMatrix.map(defaultDirection);

    // Преобразуем в rVect формат, который ожидает сервер
    rVect directVector(transformedDirection.x(), transformedDirection.y(), transformedDirection.z());
    directVector.normalize();

    return directVector;
}

void OpenGLWidget::applyFilteredTriangles(const QVector<QSharedPointer<triangle>>& filteredTriangles) {
    setTriangles(filteredTriangles);
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

    float boundingSphereRadius = std::sqrt(maxRadiusSq);

    // Обновление позиции камеры
    updateCameraPosition(center, boundingSphereRadius);

    // Обновление размеров сетки на основе ограничивающего объема
    gridSize = boundingSphereRadius * 3.0f; // Сетка 3 раза больше радиуса сферы
    gridStep = boundingSphereRadius / 10.0f; // Шаг сетки 1/10 радиуса
    // Устанавливаем флаг, что параметры сетки изменились
    gridParametersChanged = true;
}

// Получение текущей позиции камеры
QVector3D OpenGLWidget::getCameraPosition() const {
    return cameraPosition;
}

rVect OpenGLWidget::getCameraPositionAsRVect() const {
    return QVector3DToRVect(cameraPosition);
}

// Обновление позиции камеры
void OpenGLWidget::updateCameraPosition(const QVector3D& center, float radius) {
    const float fieldOfView = 45.0f;
    float viewAngleRadians = qDegreesToRadians(fieldOfView / 2.0f);
    float distance = radius / std::sin(viewAngleRadians) * 2.0f;
    cameraPosition = QVector3D(center.x(), center.y(), center.z() - distance);

    // Обновление позиции источника света, прикрепленного к камере
    lightPosition = cameraPosition + QVector3D(0.0f, 0.0f, 10.0f);

    // Устанавливаем флаг обновления сетки
    gridParametersChanged = true;

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
// void OpenGLWidget::updateVisibility(const QVector<QSharedPointer<triangle>>& triangles) {
//     rayTracer.setTriangles(triangles); // Установка треугольников для трассировщика лучей
//     rVect observerPosition = QVector3DToRVect(cameraPosition); // Преобразование QVector3D в rVect
//     rayTracer.determineVisibility(triangles, observerPosition); // Определение видимости треугольников относительно позиции камеры
// }
// Попытка увеличить прозводительность, обновляя видимость только когда камера перемещается на значительное расстояние.
void OpenGLWidget::updateVisibility(const QVector<QSharedPointer<triangle>>& triangles) {
    // Обновляем видимость только если положение камеры существенно изменилось
    static rVect lastObserverPosition;
    rVect currentObserverPosition = QVector3DToRVect(cameraPosition);

    // Если расстояние меньше порога, пропускаем обновление
    if (!triangles.isEmpty() && (lastObserverPosition - currentObserverPosition).length() < 0.1f) {
        return; // Камера мало сдвинулась, пропускаем пересчет
    }

    lastObserverPosition = currentObserverPosition;
    rayTracer.setTriangles(triangles);
    rayTracer.determineVisibility(triangles, currentObserverPosition);
}

void OpenGLWidget::setRotation(float x, float y, float z) {
    // Проверяем, действительно ли изменились значения
    if (qFuzzyCompare(rotationX, x) && qFuzzyCompare(rotationY, y) && qFuzzyCompare(rotationZ, z))
        return;

    rotationX = x;
    rotationY = y;
    rotationZ = z;
    update();
}

// Инициализация OpenGL
void OpenGLWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f); // Темно-серый фон
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

    // Инициализация цветов основных осей
    axisColors[0] = QColor::fromRgbF(1.0f, 0.0f, 0.0f); // X - Красный
    axisColors[1] = QColor::fromRgbF(0.0f, 1.0f, 0.0f); // Y - Зеленый
    axisColors[2] = QColor::fromRgbF(0.0f, 0.0f, 1.0f); // Z - Синий

    // Инициализация размеров сетки и шага
    gridSize = 1000.0f;
    gridStep = 10.0f;

    // Инициализация прозрачности сетки
    gridAlpha = maxAlpha;

    // Флаг обновления сетки
    gridParametersChanged = true;

    // Включение смешивания для прозрачности
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
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
}

// Отрисовка сцены
void OpenGLWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Применение трансформаций камеры для всей сцены
    glTranslatef(-cameraPosition.x(), -cameraPosition.y(), cameraPosition.z());

    // Установка позиции источника света, прикрепленного к камере
    GLfloat lightPos[] = { lightPosition.x(), lightPosition.y(), lightPosition.z(), 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);

    // 1. Отрисовка статичных элементов сцены (сетка и подстилающая поверхность)
    glPushMatrix(); // Сохраняем состояние матрицы для статичных элементов
    {
        // Применяем только позицию объекта, но не вращение
        glTranslatef(objectPosition.x(), objectPosition.y(), objectPosition.z());

        // Отрисовка подстилающей поверхности
        drawUnderlyingSurface();

        // Отрисовка сетки
        drawGrid();
    }
    glPopMatrix(); // Восстанавливаем состояние матрицы после отрисовки статичных элементов

    // 2. Отрисовка 3D-объекта с вращением
    glPushMatrix(); // Сохраняем состояние матрицы для объекта
    {
        // Применение трансформаций только к объекту
        glTranslatef(objectPosition.x(), objectPosition.y(), objectPosition.z());
        glScalef(scale, scale, scale);
        glRotatef(rotationX, 1.0f, 0.0f, 0.0f);
        glRotatef(rotationY, 0.0f, 1.0f, 0.0f);
        glRotatef(rotationZ, 0.0f, 0.0f, 1.0f);

        // Обновление видимости только при изменении параметров камеры или загрузке новых данных
        if (geometryChanged) {
            updateVisibility(triangles);
            geometryChanged = false; // Сбросить флаг изменения геометрии
        }

        // Отрисовка видимых треугольников
        for (int i = 0; i < triangles.size(); ++i) {
            auto triangle = triangles[i];
            if (filterShadowTriangles && !triangle->getVisible()) continue; // Пропускаем невидимые треугольники

            QColor color = triangleColors[i];
            glColor3f(color.redF(), color.greenF(), color.blueF());

            glBegin(GL_TRIANGLES);
            auto v1 = triangles[i]->getV1();
            auto v2 = triangles[i]->getV2();
            auto v3 = triangles[i]->getV3();

            rVect normal = GeometryUtils::computeNormal(triangle);
            QVector3D normalVec(normal.getX(), normal.getY(), normal.getZ());
            glNormal3f(normalVec.x(), normalVec.y(), normalVec.z());

            glVertex3f(v1->getX(), v1->getY(), v1->getZ());
            glVertex3f(v2->getX(), v2->getY(), v2->getZ());
            glVertex3f(v3->getX(), v3->getY(), v3->getZ());
            glEnd();
        }
    }
    glPopMatrix(); // Восстанавливаем состояние матрицы после отрисовки объекта

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

    hasLoadedObject = false;  // Сбрасываем флаг
    initializeDefaultGrid();  // Возвращаемся к параметрам по умолчанию

    geometryChanged = true; // Установка флага, что геометрия изменилась
    update(); // Вызов перерисовки виджета
}

// Преобразование QVector3D в rVect
rVect OpenGLWidget::QVector3DToRVect(const QVector3D& vec) const  {
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
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();

    int width = this->width();
    int height = this->height();
    glOrtho(0, width, 0, height, -100, 100);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glTranslatef(width - 60, 60, 0);
    glRotatef(-rotationZ, 0.0f, 0.0f, 1.0f);
    glRotatef(-rotationY, 0.0f, 1.0f, 0.0f);
    glRotatef(-rotationX, 1.0f, 0.0f, 0.0f);

    drawAxisWithArrow(40.0f);

    glDisable(GL_LINE_SMOOTH);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}

void OpenGLWidget::drawAxisWithArrow(float axisLength) {
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    glLineWidth(2.0f);

    // Ось X
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex3f(0, 0, 0);
    glVertex3f(axisLength, 0, 0);
    glEnd();

    // Ось Y
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINES);
    glVertex3f(0, 0, 0);
    glVertex3f(0, axisLength, 0);
    glEnd();

    // Ось Z
    glColor3f(0.0f, 0.0f, 1.0f);
    glBegin(GL_LINES);
    glVertex3f(0, 0, 0);
    glVertex3f(0, 0, axisLength);
    glEnd();

    glDisable(GL_LINE_SMOOTH);
}

void OpenGLWidget::initializeDefaultGrid() {
    // Устанавливаем параметры на основе полученных данных
    gridSize = 12.14f;
    gridStep = 0.404667f;

    // Устанавливаем начальную позицию камеры
    cameraPosition = QVector3D(-0.25f, -0.25f, -18.65f);
    lightPosition = cameraPosition + QVector3D(0.0f, 0.0f, 10.0f);

    // Устанавливаем центр и радиус по умолчанию
    boundingSphereCenter = QVector3D(-0.25f, -0.25f, 2.5f);
    boundingSphereRadius = 4.04667f;
}

void OpenGLWidget::drawGrid() {
    if (!gridVisible) return;

    // Определяем положение сетки с учетом наличия объекта
    QVector3D actualCameraPos = hasLoadedObject ? cameraPosition : QVector3D(-0.25f, -0.25f, -18.65f);
    QVector3D actualObjectPos = hasLoadedObject ? objectPosition : QVector3D(-0.25f, -0.25f, 2.5f);

    // Определяем высоту сетки
    float gridZ = 0.0f;
    if (hasLoadedObject && !vertices.isEmpty()) {
        gridZ = computeMinZ();
    }

    // Вычисление направления камеры
    QVector3D cameraDir = (actualCameraPos - actualObjectPos).normalized();

    // Нормаль плоскости XOY (Z направлен вверх)
    QVector3D gridNormal(0.0f, 0.0f, 1.0f);

    // Вычисляем косинус угла между направлением камеры и нормалью сетки
    float dotProduct = QVector3D::dotProduct(cameraDir, gridNormal);

    // Устанавливаем значение alpha
    // if (hasLoadedObject) {
    //     gridAlpha = minAlpha + (maxAlpha - minAlpha) * std::abs(dotProduct);
    //     float distance = (actualCameraPos - actualObjectPos).length();
    //     gridAlpha *= qBound(minAlpha, 1.0f - distance / 1000.0f, maxAlpha);
    // } else {
    //     gridAlpha = 0.185746f;
    // }
    gridAlpha = 0.185746f;

    // Обновление размеров сетки
    if (gridParametersChanged) {
        if (hasLoadedObject) {
            gridStep = gridSize / 20.0f;
        } else {
            gridSize = 12.14f;
            gridStep = 0.404667f;
        }
        gridParametersChanged = false;
    }

    // Включаем смешивание
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Отрисовка сетки с использованием gridAlpha
    glPushMatrix();
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT);

    glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);

    // Устанавливаем цвет сетки с альфа-прозрачностью
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA); // Для премультипликации
    glColor4f(0.5f * gridAlpha, 0.5f * gridAlpha, 0.5f * gridAlpha, gridAlpha);

    glBegin(GL_LINES);
    for (float i = -gridSize; i <= gridSize; i += gridStep) {
        // Линии параллельные X
        glVertex3f(i, -gridSize, gridZ);
        glVertex3f(i, gridSize, gridZ);

        // Линии параллельные Y
        glVertex3f(-gridSize, i, gridZ);
        glVertex3f(gridSize, i, gridZ);
    }
    glEnd();

    glPopAttrib();
    glPopMatrix();

    // Отрисовка осей координат
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_LINE_BIT);
    glLineWidth(2.0f);

    glDisable(GL_LIGHTING);

    // Ось X - красная
    glColor4f(axisColors[0].redF(), axisColors[0].greenF(), axisColors[0].blueF(), 1.0f);
    glBegin(GL_LINES);
    glVertex3f(-gridSize, 0.0f, gridZ);
    glVertex3f(gridSize, 0.0f, gridZ);
    glEnd();

    // Ось Y - зеленая
    glColor4f(axisColors[1].redF(), axisColors[1].greenF(), axisColors[1].blueF(), 1.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, -gridSize, gridZ);
    glVertex3f(0.0f, gridSize, gridZ);
    glEnd();

    // Ось Z - синяя
    glColor4f(axisColors[2].redF(), axisColors[2].greenF(), axisColors[2].blueF(), 1.0f);
    glBegin(GL_LINES);
    glVertex3f(0.0f, 0.0f, gridZ);
    glVertex3f(0.0f, 0.0f, gridZ + gridSize);
    glEnd();

    glEnable(GL_LIGHTING);
    glPopAttrib();
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

void OpenGLWidget::setUnderlyingSurfaceVisible(bool visible) {
    showUnderlyingSurface = visible;
    update();
}

void OpenGLWidget::drawUnderlyingSurface() {
    if (!showUnderlyingSurface) return;

    // Вычисляем минимальную Z-координату модели для размещения поверхности
    float minZ = computeMinZ();

    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT);

    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Временно отключаем освещение для рисования сетки
    glDisable(GL_LIGHTING);

    // Установка контрастного цвета для подстилающей поверхности
    // Используем голубоватый оттенок вместо серого
    glColor4f(0.2f, 0.4f, 0.7f, surfaceAlpha);

    // Рисуем основную поверхность
    glBegin(GL_QUADS);
    float size = gridSize * 2.0f;
    glNormal3f(0.0f, 0.0f, 1.0f); // Нормаль направлена вверх (ось Z)
    glVertex3f(-size, -size, minZ);
    glVertex3f(-size, size, minZ);
    glVertex3f(size, size, minZ);
    glVertex3f(size, -size, minZ);
    glEnd();

    // Добавляем сетчатый узор на поверхность для улучшения различимости
    glLineWidth(1.0f);
    glColor4f(0.0f, 0.2f, 0.5f, surfaceAlpha + 0.1f); // Более темный и чуть более непрозрачный цвет для сетки

    // Уменьшаем размер ячеек сетки для подстилающей поверхности
    float gridDensity = gridStep * 0.5f;

    glBegin(GL_LINES);
    for (float i = -size; i <= size; i += gridDensity) {
        // Линии параллельные X
        glVertex3f(i, -size, minZ + 0.01f); // Небольшое смещение по Z для предотвращения z-fighting
        glVertex3f(i, size, minZ + 0.01f);

        // Линии параллельные Y
        glVertex3f(-size, i, minZ + 0.01f);
        glVertex3f(size, i, minZ + 0.01f);
    }
    glEnd();

    // Добавляем выделение границы поверхности
    glLineWidth(2.0f);
    glColor4f(0.0f, 0.3f, 0.6f, surfaceAlpha + 0.2f); // Еще более темный цвет для границы
    glBegin(GL_LINE_LOOP);
    glVertex3f(-size, -size, minZ + 0.02f);
    glVertex3f(-size, size, minZ + 0.02f);
    glVertex3f(size, size, minZ + 0.02f);
    glVertex3f(size, -size, minZ + 0.02f);
    glEnd();

    // Восстанавливаем атрибуты
    glPopAttrib();
}

// Функция для нахождения минимальной Z-координаты модели
float OpenGLWidget::computeMinZ() const {
    if (vertices.isEmpty()) {
        return hasLoadedObject ? 0.0f : -0.5f;
    }

    float minZ = vertices[0].z();
    for (const auto& vertex : vertices) {
        minZ = qMin(minZ, vertex.z());
    }

    // Добавляем небольшой отступ, чтобы поверхность гарантированно была под объектом
    return minZ - 0.5f;
}

void OpenGLWidget::getRotation(float &x, float &y, float &z) const {
    x = rotationX;
    y = rotationY;
    z = rotationZ;
}
