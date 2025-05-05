#include "patterndiagramwindow.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QtMath>
#include <algorithm>
#include <QDebug>
#include <QMatrix4x4>
#include <QOpenGLShaderProgram>
#include <QOpenGLBuffer>
#include <QOpenGLVertexArrayObject>
#include <QGroupBox>
#include <QKeyEvent>

GLPatternWidget::GLPatternWidget(QWidget *parent)
    : QOpenGLWidget(parent),
    xRot(0.0f),
    yRot(0.0f),
    zRot(0.0f),
    scale(1.0f),
    threshold(0.1f),
    colorMode(0),
    viewMode(PointsMode), // По умолчанию - режим точек
    sliceIndex(0), // По умолчанию - первый срез
    maxValue(1.0),
    minValue(0.0)
{
    setFocusPolicy(Qt::StrongFocus);
}

void GLPatternWidget::setData(const QVector<QVector<QVector<double>>> &data)
{
    data3D = data;

    // Find max and min values for color mapping
    maxValue = 0.0;
    minValue = std::numeric_limits<double>::max();

    for (const auto &plane : data3D) {
        for (const auto &row : plane) {
            for (double val : row) {
                maxValue = qMax(maxValue, val);
                minValue = qMin(minValue, val);
            }
        }
    }

    // Ensure we have a non-zero range
    if (qFuzzyCompare(maxValue, minValue)) {
        maxValue = minValue + 1.0;
    }

    // Generate data for all view modes
    updateVertexBuffer();
    generateSurfaceMesh();
    updateSliceData();
    generateSphericalMesh();

    update();
}

void GLPatternWidget::setThreshold(double value)
{
    threshold = value;
    updateVertexBuffer();
    update();
}

void GLPatternWidget::setColorMode(int mode)
{
    colorMode = mode;
    updateVertexBuffer();
    update();
}

void GLPatternWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_POINT_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void GLPatternWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (data3D.isEmpty() || data3D[0].isEmpty() || data3D[0][0].isEmpty()) {
        return;
    }

    // Set up camera view
    QMatrix4x4 mvpMatrix;
    mvpMatrix.perspective(45.0f, (float)width() / height(), 0.1f, 100.0f);
    mvpMatrix.translate(0.0f, 0.0f, -5.0f);
    mvpMatrix.scale(scale, scale, scale);
    mvpMatrix.rotate(xRot, 1.0f, 0.0f, 0.0f);
    mvpMatrix.rotate(yRot, 0.0f, 1.0f, 0.0f);
    mvpMatrix.rotate(zRot, 0.0f, 0.0f, 1.0f);

    // Different centering for sphere mode
    float xCenter, yCenter, zCenter;
    if (viewMode == SphereMode) {
        // For sphere mode, don't center based on data dimensions
        xCenter = 0.0f;
        yCenter = 0.0f;
        zCenter = 0.0f;
    } else {
        // Center for other modes
        xCenter = -0.5f * data3D[0][0].size();
        yCenter = -0.5f * data3D[0].size();
        zCenter = -0.5f * data3D.size();
    }
    mvpMatrix.translate(xCenter, yCenter, zCenter);

    glPushMatrix();
    glLoadMatrixf(mvpMatrix.data());

    // Отрисовка в зависимости от режима
    switch (viewMode) {
    case PointsMode:
        if (!vertices.isEmpty()) {
            // Режим точек - как было раньше
            glPointSize(3.0f);
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);

            glVertexPointer(3, GL_FLOAT, 6 * sizeof(GLfloat), vertices.data());
            glColorPointer(3, GL_FLOAT, 6 * sizeof(GLfloat), vertices.data() + 3);

            glDrawArrays(GL_POINTS, 0, vertices.size() / 6);

            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_COLOR_ARRAY);
        }
        break;

    case SurfaceMode:
        if (!surfaceVertices.isEmpty()) {
            // Режим поверхности - рисуем треугольники
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);

            glVertexPointer(3, GL_FLOAT, 6 * sizeof(GLfloat), surfaceVertices.data());
            glColorPointer(3, GL_FLOAT, 6 * sizeof(GLfloat), surfaceVertices.data() + 3);

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawArrays(GL_TRIANGLES, 0, surfaceVertices.size() / 6);

            // Дополнительно можно нарисовать каркас поверх поверхности
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glColor3f(0.5f, 0.5f, 0.5f);
            glDrawArrays(GL_TRIANGLES, 0, surfaceVertices.size() / 6);

            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_COLOR_ARRAY);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // Возвращаем режим по умолчанию
        }
        break;

    case SliceMode:
        if (!sliceVertices.isEmpty()) {
            // Режим срезов - рисуем текстурированный квад
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);

            glVertexPointer(3, GL_FLOAT, 6 * sizeof(GLfloat), sliceVertices.data());
            glColorPointer(3, GL_FLOAT, 6 * sizeof(GLfloat), sliceVertices.data() + 3);

            glDrawArrays(GL_QUADS, 0, sliceVertices.size() / 6);

            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_COLOR_ARRAY);
        }
        break;

    case SphereMode:
        if (!sphereVertices.isEmpty()) {
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);

            glVertexPointer(3, GL_FLOAT, 6 * sizeof(GLfloat), sphereVertices.data());
            glColorPointer(3, GL_FLOAT, 6 * sizeof(GLfloat), sphereVertices.data() + 3);

            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawArrays(GL_TRIANGLES, 0, sphereVertices.size() / 6);

            // Optional: Draw wireframe on top
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
            glColor3f(0.3f, 0.3f, 0.3f);
            glDrawArrays(GL_TRIANGLES, 0, sphereVertices.size() / 6);

            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_COLOR_ARRAY);
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }
        break;
    }

    glPopMatrix();

    // Draw coordinate axes
    glLineWidth(2.0f);
    glBegin(GL_LINES);
    // X axis (red)
    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(xCenter, yCenter, zCenter);
    glVertex3f(xCenter + data3D[0][0].size(), yCenter, zCenter);

    // Y axis (green)
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(xCenter, yCenter, zCenter);
    glVertex3f(xCenter, yCenter + data3D[0].size(), zCenter);

    // Z axis (blue)
    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(xCenter, yCenter, zCenter);
    glVertex3f(xCenter, yCenter, zCenter + data3D.size());
    glEnd();
}

void GLPatternWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void GLPatternWidget::mousePressEvent(QMouseEvent *event)
{
    lastMousePos = event->pos();
}

void GLPatternWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx = event->pos().x() - lastMousePos.x();
    int dy = event->pos().y() - lastMousePos.y();

    if (event->buttons() & Qt::LeftButton) {
        yRot += dx;
        xRot += dy;
        update();
    } else if (event->buttons() & Qt::RightButton) {
        zRot += dx;
        update();
    }

    lastMousePos = event->pos();
}

void GLPatternWidget::wheelEvent(QWheelEvent *event)
{
    QPoint numDegrees = event->angleDelta() / 8;

    if (!numDegrees.isNull()) {
        scale *= std::pow(1.1, numDegrees.y() / 15.0);
        scale = qBound(0.1f, scale, 10.0f);
        update();
    }

    event->accept();
}

void GLPatternWidget::updateVertexBuffer()
{
    vertices.clear();

    if (data3D.isEmpty() || data3D[0].isEmpty() || data3D[0][0].isEmpty()) {
        return;
    }

    int zSize = data3D.size();
    int ySize = data3D[0].size();
    int xSize = data3D[0][0].size();

    // Scale factors to normalize coordinates
    float xScale = 1.0f;
    float yScale = (float)xSize / ySize;
    float zScale = (float)xSize / zSize;

    for (int z = 0; z < zSize; z++) {
        for (int y = 0; y < ySize; y++) {
            for (int x = 0; x < xSize; x++) {
                double value = data3D[z][y][x];

                // Skip points below threshold
                if (value < threshold * maxValue) {
                    continue;
                }

                // Add vertex
                vertices.append(x * xScale);
                vertices.append(y * yScale);
                vertices.append(z * zScale);

                // Add color
                QColor color = getColorForValue(value);
                vertices.append(color.redF());
                vertices.append(color.greenF());
                vertices.append(color.blueF());
            }
        }
    }
}

QColor GLPatternWidget::getColorForValue(double value) const
{
    // Normalize value between 0 and 1
    double normValue = (value - minValue) / (maxValue - minValue);
    normValue = qBound(0.0, normValue, 1.0);

    switch (colorMode) {
    case 0: // HSV spectrum (blue to red)
        return QColor::fromHsv(240 - (int)(240 * normValue), 255, 255);

    case 1: // Grayscale
    {
        int gray = (int)(255 * normValue);
        return QColor(gray, gray, gray);
    }

    case 2: // Hot (black to red to yellow to white)
    {
        if (normValue < 0.33) {
            // Black to red
            return QColor((int)(255 * (normValue * 3)), 0, 0);
        } else if (normValue < 0.67) {
            // Red to yellow
            return QColor(255, (int)(255 * ((normValue - 0.33) * 3)), 0);
        } else {
            // Yellow to white
            return QColor(255, 255, (int)(255 * ((normValue - 0.67) * 3)));
        }
    }

    default:
        return QColor::fromHsv(240 - (int)(240 * normValue), 255, 255);
    }
}

// PatternDiagramWindow implementation
PatternDiagramWindow::PatternDiagramWindow(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Трехмерная диаграмма"));
    resize(1024, 768);
    setupUI();
}

void PatternDiagramWindow::setupUI()
{
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Create OpenGL widget for 3D visualization
    glWidget = new GLPatternWidget(this);

    // Create controls
    QHBoxLayout *controlsLayout = new QHBoxLayout();

    // View mode group
    QGroupBox *viewModeGroup = new QGroupBox(tr("Режим отображения"));
    QVBoxLayout *viewModeLayout = new QVBoxLayout(viewModeGroup);

    viewModeCombo = new QComboBox();
    viewModeCombo->addItem(tr("3D точки"));
    viewModeCombo->addItem(tr("3D поверхность"));
    viewModeCombo->addItem(tr("2D срезы"));
    viewModeCombo->addItem(tr("Сферическая диаграмма"));

    viewModeLayout->addWidget(viewModeCombo);

    // Color mode group
    QGroupBox *colorModeGroup = new QGroupBox(tr("Цветовая схема"));
    QVBoxLayout *colorModeLayout = new QVBoxLayout(colorModeGroup);

    colorModeCombo = new QComboBox();
    colorModeCombo->addItem(tr("Спектр HSV"));
    colorModeCombo->addItem(tr("Оттенки серого"));
    colorModeCombo->addItem(tr("Тепловая карта"));

    colorModeLayout->addWidget(colorModeCombo);

    // Threshold group
    QGroupBox *thresholdGroup = new QGroupBox(tr("Порог отображения"));
    QVBoxLayout *thresholdLayout = new QVBoxLayout(thresholdGroup);

    thresholdSlider = new QSlider(Qt::Horizontal);
    thresholdSlider->setRange(0, 100);
    thresholdSlider->setValue(10); // 10% by default

    thresholdLayout->addWidget(thresholdSlider);

    // Slice control group - ADD THIS GROUP
    sliceControlGroup = new QGroupBox(tr("Управление срезами"));
    QVBoxLayout *sliceControlLayout = new QVBoxLayout(sliceControlGroup);

    sliceLabel = new QLabel(tr("Срез по Z:"));
    sliceSlider = new QSlider(Qt::Horizontal);
    sliceSlider->setRange(0, 100);
    sliceSlider->setValue(50); // Середина по умолчанию

    sliceControlLayout->addWidget(sliceLabel);
    sliceControlLayout->addWidget(sliceSlider);

    // Сначала делаем его невидимым, покажем только в режиме срезов
    sliceControlGroup->setVisible(false);

    // Buttons group
    QGroupBox *buttonsGroup = new QGroupBox(tr("Действия"));
    QVBoxLayout *buttonsLayout = new QVBoxLayout(buttonsGroup);

    saveButton = new QPushButton(tr("Сохранить изображение"));
    resetViewButton = new QPushButton(tr("Сбросить вид"));

    buttonsLayout->addWidget(saveButton);
    buttonsLayout->addWidget(resetViewButton);

    // Add all control groups
    controlsLayout->addWidget(viewModeGroup);
    controlsLayout->addWidget(colorModeGroup);
    controlsLayout->addWidget(thresholdGroup);
    controlsLayout->addWidget(sliceControlGroup); // ADD THIS
    controlsLayout->addWidget(buttonsGroup);

    // Add widgets to main layout
    mainLayout->addWidget(glWidget, 1);
    mainLayout->addLayout(controlsLayout);

    // Connect signals
    connect(saveButton, &QPushButton::clicked, this, &PatternDiagramWindow::savePatternAsPNG);
    connect(resetViewButton, &QPushButton::clicked, this, &PatternDiagramWindow::resetView);
    connect(thresholdSlider, &QSlider::valueChanged, this, &PatternDiagramWindow::onThresholdChanged);
    connect(colorModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PatternDiagramWindow::onColorModeChanged);
    connect(viewModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &PatternDiagramWindow::onViewModeChanged);
    connect(sliceSlider, &QSlider::valueChanged, this, &PatternDiagramWindow::onSliceChanged); // ADD THIS
}

void PatternDiagramWindow::setData(const QVector<QVector<QVector<double>>> &data)
{
    data3D = data;
    glWidget->setData(data);
}

void PatternDiagramWindow::setData(const QVector<QVector<double>> &data2D)
{
    // Convert 2D data to 3D for visualization
    convert2DTo3D(data2D);
    glWidget->setData(data3D);
}

void PatternDiagramWindow::convert2DTo3D(const QVector<QVector<double>> &data2D)
{
    if (data2D.isEmpty()) return;

    int rows = data2D.size();
    int cols = data2D[0].size();

    // Create a 3D dataset with copies of the 2D data along the Z axis
    // This creates a "cylindrical" visualization
    data3D.clear();

    int zSize = qMin(rows, cols) / 2; // Use a reasonable depth

    for (int z = 0; z < zSize; z++) {
        double zFactor = (double)z / zSize; // Scale factor for depth
        QVector<QVector<double>> layer;

        for (int y = 0; y < rows; y++) {
            QVector<double> row;
            for (int x = 0; x < cols; x++) {
                double value = data2D[y][x] * (1.0 - zFactor * 0.9); // Fade with depth
                row.append(value);
            }
            layer.append(row);
        }

        data3D.append(layer);
    }
}

void PatternDiagramWindow::savePatternAsPNG()
{
    QString fileName = QFileDialog::getSaveFileName(this, tr("Сохранить изображение"),
                                                    "", tr("PNG Files (*.png)"));
    if (fileName.isEmpty()) {
        return;
    }

    // Get the OpenGL frame as an image
    QImage image = glWidget->grabFramebuffer();

    if (!image.save(fileName, "PNG")) {
        QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось сохранить файл."));
    } else {
        QMessageBox::information(this, tr("Успех"), tr("Изображение успешно сохранено."));
    }
}

void PatternDiagramWindow::onThresholdChanged(int value)
{
    double threshold = value / 100.0;
    glWidget->setThreshold(threshold);
}

void PatternDiagramWindow::onColorModeChanged(int index)
{
    glWidget->setColorMode(index);
}

void PatternDiagramWindow::onViewModeChanged(int index)
{
    // Преобразуем индекс в enum GLPatternWidget::ViewMode
    GLPatternWidget::ViewMode mode = static_cast<GLPatternWidget::ViewMode>(index);

    // Обновляем режим отображения
    glWidget->setViewMode(mode);

    // Включаем/отключаем элементы управления в зависимости от режима
    if (mode == GLPatternWidget::SliceMode) {
        // Для режима срезов нам нужен дополнительный слайдер для выбора среза
        // Если его нет, создаем и добавляем в интерфейс
        // ...
    }
    // Показываем/скрываем элементы управления срезами
    bool isSliceMode = (index == GLPatternWidget::SliceMode);
    sliceControlGroup->setVisible(isSliceMode);

    // Если перешли в режим срезов, инициализируем слайдер
    if (isSliceMode) {
        // Перенастраиваем слайдер в зависимости от количества срезов
        int maxSlices = data3D.size() > 0 ? data3D.size() - 1 : 0;
        int currentSlice = maxSlices / 2; // По умолчанию - середина

        // Устанавливаем значение слайдера
        sliceSlider->blockSignals(true);
        sliceSlider->setValue((currentSlice * 100) / maxSlices);
        sliceSlider->blockSignals(false);

        // Устанавливаем текущий срез
        if (glWidget) {
            glWidget->setSliceIndex(currentSlice);
        }
    }

}

// Метод для генерации данных поверхности
void GLPatternWidget::generateSurfaceMesh()
{
    surfaceVertices.clear();

    if (data3D.isEmpty() || data3D[0].isEmpty() || data3D[0][0].isEmpty()) {
        return;
    }

    int zSize = data3D.size();
    int ySize = data3D[0].size();
    int xSize = data3D[0][0].size();

    // Scale factors to normalize coordinates
    float xScale = 1.0f;
    float yScale = (float)xSize / ySize;
    float zScale = (float)xSize / zSize;

    // Создаем треугольную сетку
    for (int z = 0; z < zSize - 1; z++) {
        for (int y = 0; y < ySize - 1; y++) {
            for (int x = 0; x < xSize - 1; x++) {
                double value1 = data3D[z][y][x];
                double value2 = data3D[z][y+1][x];
                double value3 = data3D[z+1][y][x];
                double value4 = data3D[z+1][y+1][x];

                // Пропускаем точки ниже порога
                if (value1 < threshold * maxValue &&
                    value2 < threshold * maxValue &&
                    value3 < threshold * maxValue &&
                    value4 < threshold * maxValue) {
                    continue;
                }

                // Треугольник 1
                // Вершина 1
                surfaceVertices.append(x * xScale);
                surfaceVertices.append(y * yScale);
                surfaceVertices.append(z * zScale);

                QColor color1 = getColorForValue(value1);
                surfaceVertices.append(color1.redF());
                surfaceVertices.append(color1.greenF());
                surfaceVertices.append(color1.blueF());

                // Вершина 2
                surfaceVertices.append(x * xScale);
                surfaceVertices.append((y+1) * yScale);
                surfaceVertices.append(z * zScale);

                QColor color2 = getColorForValue(value2);
                surfaceVertices.append(color2.redF());
                surfaceVertices.append(color2.greenF());
                surfaceVertices.append(color2.blueF());

                // Вершина 3
                surfaceVertices.append((x+1) * xScale);
                surfaceVertices.append(y * yScale);
                surfaceVertices.append(z * zScale);

                QColor color3 = getColorForValue(value3);
                surfaceVertices.append(color3.redF());
                surfaceVertices.append(color3.greenF());
                surfaceVertices.append(color3.blueF());

                // Треугольник 2
                // Вершина 1
                surfaceVertices.append((x+1) * xScale);
                surfaceVertices.append(y * yScale);
                surfaceVertices.append(z * zScale);

                surfaceVertices.append(color3.redF());
                surfaceVertices.append(color3.greenF());
                surfaceVertices.append(color3.blueF());

                // Вершина 2
                surfaceVertices.append(x * xScale);
                surfaceVertices.append((y+1) * yScale);
                surfaceVertices.append(z * zScale);

                surfaceVertices.append(color2.redF());
                surfaceVertices.append(color2.greenF());
                surfaceVertices.append(color2.blueF());

                // Вершина 3
                surfaceVertices.append((x+1) * xScale);
                surfaceVertices.append((y+1) * yScale);
                surfaceVertices.append(z * zScale);

                QColor color4 = getColorForValue(value4);
                surfaceVertices.append(color4.redF());
                surfaceVertices.append(color4.greenF());
                surfaceVertices.append(color4.blueF());
            }
        }
    }
}

// Метод для подготовки данных 2D срезов
void GLPatternWidget::updateSliceData()
{
    sliceVertices.clear();

    if (data3D.isEmpty() || data3D[0].isEmpty() || data3D[0][0].isEmpty()) {
        return;
    }

    int zSize = data3D.size();
    int ySize = data3D[0].size();
    int xSize = data3D[0][0].size();

    // Нормируем координаты
    float xScale = 1.0f;
    float yScale = (float)xSize / ySize;
    float zScale = (float)xSize / zSize;

    // Ограничиваем индекс среза в пределах размера данных
    sliceIndex = qBound(0, sliceIndex, zSize - 1);

    // Создаем XY-срез для текущего Z
    for (int y = 0; y < ySize - 1; y++) {
        for (int x = 0; x < xSize - 1; x++) {
            double value1 = data3D[sliceIndex][y][x];
            double value2 = data3D[sliceIndex][y+1][x];
            double value3 = data3D[sliceIndex][y][x+1];
            double value4 = data3D[sliceIndex][y+1][x+1];

            // Пропускаем квады с низкой интенсивностью
            if (value1 < threshold * maxValue &&
                value2 < threshold * maxValue &&
                value3 < threshold * maxValue &&
                value4 < threshold * maxValue) {
                continue;
            }

            // Вершина 1
            sliceVertices.append(x * xScale);
            sliceVertices.append(y * yScale);
            sliceVertices.append(sliceIndex * zScale);

            QColor color1 = getColorForValue(value1);
            sliceVertices.append(color1.redF());
            sliceVertices.append(color1.greenF());
            sliceVertices.append(color1.blueF());

            // Вершина 2
            sliceVertices.append((x+1) * xScale);
            sliceVertices.append(y * yScale);
            sliceVertices.append(sliceIndex * zScale);

            QColor color3 = getColorForValue(value3);
            sliceVertices.append(color3.redF());
            sliceVertices.append(color3.greenF());
            sliceVertices.append(color3.blueF());

            // Вершина 3
            sliceVertices.append((x+1) * xScale);
            sliceVertices.append((y+1) * yScale);
            sliceVertices.append(sliceIndex * zScale);

            QColor color4 = getColorForValue(value4);
            sliceVertices.append(color4.redF());
            sliceVertices.append(color4.greenF());
            sliceVertices.append(color4.blueF());

            // Вершина 4
            sliceVertices.append(x * xScale);
            sliceVertices.append((y+1) * yScale);
            sliceVertices.append(sliceIndex * zScale);

            QColor color2 = getColorForValue(value2);
            sliceVertices.append(color2.redF());
            sliceVertices.append(color2.greenF());
            sliceVertices.append(color2.blueF());
        }
    }
}

// Метод для сброса вида
void GLPatternWidget::resetView()
{
    xRot = 0.0f;
    yRot = 0.0f;
    zRot = 0.0f;
    scale = 1.0f;

    // При сбросе пересоздаем все буферы
    updateVertexBuffer();
    if (viewMode == SurfaceMode) {
        generateSurfaceMesh();
    } else if (viewMode == SliceMode) {
        updateSliceData();
    }
    update();
}

// Метод для установки режима отображения
void GLPatternWidget::setViewMode(ViewMode mode)
{
    if (viewMode != mode) {
        viewMode = mode;
        // При изменении режима обновляем буферы для нового режима
        if (mode == SurfaceMode) {
            generateSurfaceMesh();
        } else if (mode == SliceMode) {
            updateSliceData();
        } else if (mode == SphereMode) {
            // Reset view for sphere mode to ensure visibility
            xRot = 0.0f;
            yRot = 0.0f;
            zRot = 0.0f;
            scale = 1.0f;  // Reset scale
            generateSphericalMesh();
        }
        update();
    }
}

void PatternDiagramWindow::resetView()
{
    // Сбрасываем вид
    glWidget->resetView();

    // Сбрасываем значение слайдера порога
    thresholdSlider->setValue(10);
}

void GLPatternWidget::setSliceIndex(int index)
{
    if (data3D.isEmpty()) return;

    // Ограничиваем индекс в пределах размера данных
    int maxIndex = data3D.size() - 1;
    sliceIndex = qBound(0, index, maxIndex);

    // Обновляем данные для слайса и перерисовываем
    if (viewMode == SliceMode) {
        updateSliceData();
        update();
    }
}

void PatternDiagramWindow::onSliceChanged(int value)
{
    // Преобразуем значение слайдера в индекс среза
    int maxSlices = data3D.size() > 0 ? data3D.size() - 1 : 0;
    int sliceIndex = (value * maxSlices) / 100;

    // Устанавливаем индекс среза в GLPatternWidget
    if (glWidget) {
        glWidget->setSliceIndex(sliceIndex);
    }
}

void GLPatternWidget::keyPressEvent(QKeyEvent *event)
{
    if (viewMode == SliceMode) {
        switch (event->key()) {
        case Qt::Key_Up:
        case Qt::Key_PageUp:
            setSliceIndex(sliceIndex + 1);
            break;

        case Qt::Key_Down:
        case Qt::Key_PageDown:
            setSliceIndex(sliceIndex - 1);
            break;

        case Qt::Key_Home:
            setSliceIndex(0);
            break;

        case Qt::Key_End:
            setSliceIndex(data3D.size() - 1);
            break;
        }
    }

    QOpenGLWidget::keyPressEvent(event);
}

void GLPatternWidget::generateSphericalMesh()
{
    sphereVertices.clear();

    if (data3D.isEmpty() || data3D[0].isEmpty() || data3D[0][0].isEmpty()) {
        qDebug() << "No data for spherical mesh";
        return;
    }

    // In this mode, we'll interpret:
    // data3D[i][j][k] where i=угломестный, j=азимутальный, k=дальностный
    int thetaSteps = data3D.size();        // угломестный (0 to 180 degrees)
    int phiSteps = data3D[0].size();       // азимутальный (0 to 360 degrees)
    int rangeSteps = data3D[0][0].size();  // дальностный

    qDebug() << "Generating spherical mesh with dimensions:"
             << thetaSteps << "theta steps,"
             << phiSteps << "phi steps,"
             << rangeSteps << "range steps";

    double dtheta = M_PI / (thetaSteps > 1 ? thetaSteps - 1 : 1);
    double dphi = 2.0 * M_PI / (phiSteps > 1 ? phiSteps - 1 : 1);

    // Add a scaling factor to make the sphere more visible
    float sphereScale = 5.0f;  // Increased scale for better visibility

    // Lower the threshold for visualization
    float visualThreshold = threshold * 0.5;

    // Count generated vertices for debugging
    int generatedVertices = 0;

    // Create triangular mesh
    for (int i = 0; i < thetaSteps - 1; i++) {
        double theta1 = i * dtheta;
        double theta2 = (i + 1) * dtheta;

        for (int j = 0; j < phiSteps - 1; j++) {
            double phi1 = j * dphi;
            double phi2 = (j + 1) * dphi;

            // Aggregate data along range dimension using maximum value
            double val1 = 0.0;
            double val2 = 0.0;
            double val3 = 0.0;
            double val4 = 0.0;

            for (int k = 0; k < rangeSteps; k++) {
                val1 = qMax(val1, data3D[i][j][k]);
                val2 = qMax(val2, data3D[i][j+1][k]);
                val3 = qMax(val3, data3D[i+1][j+1][k]);
                val4 = qMax(val4, data3D[i+1][j][k]);
            }

            // Apply threshold
            if (val1 < visualThreshold * maxValue &&
                val2 < visualThreshold * maxValue &&
                val3 < visualThreshold * maxValue &&
                val4 < visualThreshold * maxValue) {
                continue;
            }

            // Scale by intensity
            val1 = qMax(0.1, val1 / maxValue) * sphereScale;
            val2 = qMax(0.1, val2 / maxValue) * sphereScale;
            val3 = qMax(0.1, val3 / maxValue) * sphereScale;
            val4 = qMax(0.1, val4 / maxValue) * sphereScale;

            // Calculate vertices in 3D space (spherical to Cartesian)
            float x1 = val1 * sin(theta1) * cos(phi1);
            float y1 = val1 * sin(theta1) * sin(phi1);
            float z1 = val1 * cos(theta1);

            float x2 = val2 * sin(theta1) * cos(phi2);
            float y2 = val2 * sin(theta1) * sin(phi2);
            float z2 = val2 * cos(theta1);

            float x3 = val3 * sin(theta2) * cos(phi2);
            float y3 = val3 * sin(theta2) * sin(phi2);
            float z3 = val3 * cos(theta2);

            float x4 = val4 * sin(theta2) * cos(phi1);
            float y4 = val4 * sin(theta2) * sin(phi1);
            float z4 = val4 * cos(theta2);

            // Colors for each vertex
            QColor color1 = getColorForValue(val1 * maxValue / sphereScale);
            QColor color2 = getColorForValue(val2 * maxValue / sphereScale);
            QColor color3 = getColorForValue(val3 * maxValue / sphereScale);
            QColor color4 = getColorForValue(val4 * maxValue / sphereScale);

            // Add triangles to the mesh
            // Triangle 1
            sphereVertices.append(x1);
            sphereVertices.append(y1);
            sphereVertices.append(z1);
            sphereVertices.append(color1.redF());
            sphereVertices.append(color1.greenF());
            sphereVertices.append(color1.blueF());

            sphereVertices.append(x2);
            sphereVertices.append(y2);
            sphereVertices.append(z2);
            sphereVertices.append(color2.redF());
            sphereVertices.append(color2.greenF());
            sphereVertices.append(color2.blueF());

            sphereVertices.append(x3);
            sphereVertices.append(y3);
            sphereVertices.append(z3);
            sphereVertices.append(color3.redF());
            sphereVertices.append(color3.greenF());
            sphereVertices.append(color3.blueF());

            // Triangle 2
            sphereVertices.append(x1);
            sphereVertices.append(y1);
            sphereVertices.append(z1);
            sphereVertices.append(color1.redF());
            sphereVertices.append(color1.greenF());
            sphereVertices.append(color1.blueF());

            sphereVertices.append(x3);
            sphereVertices.append(y3);
            sphereVertices.append(z3);
            sphereVertices.append(color3.redF());
            sphereVertices.append(color3.greenF());
            sphereVertices.append(color3.blueF());

            sphereVertices.append(x4);
            sphereVertices.append(y4);
            sphereVertices.append(z4);
            sphereVertices.append(color4.redF());
            sphereVertices.append(color4.greenF());
            sphereVertices.append(color4.blueF());

            generatedVertices += 6;  // Added 6 vertices (2 triangles)
        }
    }

    qDebug() << "Generated spherical mesh with" << generatedVertices << "vertices";
    qDebug() << "sphereVertices.size() = " << sphereVertices.size();
}
