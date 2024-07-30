#include "scatterplot3dwindow.h"
#include <QMatrix4x4>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QDebug>
#include <QVBoxLayout>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <algorithm>

ScatterPlot3DWidget::ScatterPlot3DWidget(QWidget *parent)
    : QOpenGLWidget(parent), rotationX(0.0f), rotationY(0.0f), rotationZ(0.0f), scale(1.0f) {
    setFocusPolicy(Qt::StrongFocus);
}

void ScatterPlot3DWidget::setData(const QVector<QVector3D> &vertices, const QVector<QVector<int>> &indices) {
    this->vertices = vertices;
    this->indices = indices;
    update(); // Перерисовать окно
}

void ScatterPlot3DWidget::setScatteringData(const QVector<double> &absEout, const QVector<double> &normEout) {
    this->absEout = absEout;
    this->normEout = normEout;

    // Проверка совпадения размеров данных и количества вершин
    if (this->absEout.size() != vertices.size() || this->normEout.size() != vertices.size()) {
        qDebug() << "Предупреждение: Размеры absEout или normEout не совпадают с количеством вершин.";
        qDebug() << "Размер absEout:" << this->absEout.size() << "Размер normEout:" << this->normEout.size() << "Количество вершин:" << vertices.size();

        if (this->absEout.isEmpty() || this->normEout.isEmpty()) {
            qDebug() << "Ошибка: Данные absEout или normEout пусты.";
            return;
        }

        // Масштабирование данных до размера количества вершин
        QVector<double> scaledAbsEout(vertices.size());
        QVector<double> scaledNormEout(vertices.size());

        for (int i = 0; i < vertices.size(); ++i) {
            int idx = i % this->absEout.size();
            scaledAbsEout[i] = this->absEout[idx];
            scaledNormEout[i] = this->normEout[idx];
        }

        this->absEout = scaledAbsEout;
        this->normEout = scaledNormEout;
        qDebug() << "Масштабирование данных выполнено. Новый размер absEout:" << this->absEout.size() << "Новый размер normEout:" << this->normEout.size();
    }

    // Проверка данных после масштабирования
    for (int i = 0; i < vertices.size(); ++i) {
        qDebug() << "Вершина" << i << ": Координаты" << vertices[i]
                 << "absEout:" << this->absEout[i]
                 << "normEout:" << this->normEout[i];
    }

    scatteringDataSet = true; // Установка флага при загрузке данных
    update(); // Перерисовать окно
}

void ScatterPlot3DWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.0, 0.0, 0.0, 1.0); // Черный фон
    glEnable(GL_DEPTH_TEST); // Включить тест глубины
}

void ScatterPlot3DWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
    projection.setToIdentity();
    projection.perspective(45.0f, GLfloat(w) / h, 0.1f, 1000.0f);
}

void ScatterPlot3DWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    QMatrix4x4 modelView;
    modelView.translate(0.0, 0.0, -5.0);
    modelView.rotate(rotationX, 1.0, 0.0, 0.0);
    modelView.rotate(rotationY, 0.0, 1.0, 0.0);
    modelView.rotate(rotationZ, 0.0, 0.0, 1.0);
    modelView.scale(scale, scale, scale);

    QMatrix4x4 mvpMatrix = projection * modelView;
    glLoadMatrixf(mvpMatrix.constData());

    drawScatterPlot();
    drawScattering();
}

void ScatterPlot3DWidget::drawScatterPlot() {
    if (vertices.isEmpty())
        return;

    qDebug() << "Отрисовка треугольников:";
    glBegin(GL_TRIANGLES);
    for (const auto &index : indices) {
        if (index.size() == 3) {
            int idx1 = index[0];
            int idx2 = index[1];
            int idx3 = index[2];
            if (idx1 < vertices.size() && idx2 < vertices.size() && idx3 < vertices.size()) {
                glColor3f(1.0f, 1.0f, 1.0f); // Белый цвет для треугольников
                glVertex3f(vertices[idx1].x(), vertices[idx1].y(), vertices[idx1].z());
                glVertex3f(vertices[idx2].x(), vertices[idx2].y(), vertices[idx2].z());
                glVertex3f(vertices[idx3].x(), vertices[idx3].y(), vertices[idx3].z());

                // Вывод данных в консоль
                qDebug() << "Треугольник:" << idx1 << vertices[idx1] << idx2 << vertices[idx2] << idx3 << vertices[idx3];
            }
        }
    }
    glEnd();
}

void ScatterPlot3DWidget::drawScattering() {
    if (absEout.isEmpty() || normEout.isEmpty() || !scatteringDataSet)
        return;

    double maxAbsEout = *std::max_element(absEout.begin(), absEout.end());
    double minAbsEout = *std::min_element(absEout.begin(), absEout.end());

    qDebug() << "Отрисовка распределения электромагнитных волн:";
    glBegin(GL_TRIANGLES);
    for (const auto &index : indices) {
        if (index.size() == 3) {
            int idx1 = index[0];
            int idx2 = index[1];
            int idx3 = index[2];

            if (idx1 < vertices.size() && idx2 < vertices.size() && idx3 < vertices.size()) {
                auto setColor = [&](int idx) {
                    double normalizedVal = (absEout[idx] - minAbsEout) / (maxAbsEout - minAbsEout);
                    QColor color = QColor::fromHsvF(0.67 - 0.67 * normalizedVal, 1.0, 1.0); // От синего к красному
                    glColor3f(color.redF(), color.greenF(), color.blueF());
                };

                setColor(idx1);
                glVertex3f(vertices[idx1].x(), vertices[idx1].y(), vertices[idx1].z());
                setColor(idx2);
                glVertex3f(vertices[idx2].x(), vertices[idx2].y(), vertices[idx2].z());
                setColor(idx3);
                glVertex3f(vertices[idx3].x(), vertices[idx3].y(), vertices[idx3].z());

                // Вывод данных в консоль
                qDebug() << "Треугольник:" << idx1 << vertices[idx1] << idx2 << vertices[idx2] << idx3 << vertices[idx3]
                         << "absEout:" << absEout[idx1] << absEout[idx2] << absEout[idx3];
            }
        }
    }
    glEnd();
}

void ScatterPlot3DWidget::mousePressEvent(QMouseEvent *event) {
    lastMousePosition = event->pos();
}

void ScatterPlot3DWidget::mouseMoveEvent(QMouseEvent *event) {
    float dx = event->pos().x() - lastMousePosition.x();
    float dy = event->pos().y() - lastMousePosition.y();

    if (event->buttons() & Qt::LeftButton) {
        rotationX += dy * 0.5f;
        rotationY += dx * 0.5f;
    } else if (event->buttons() & Qt::RightButton) {
        rotationZ += dx * 0.5f;
    }

    update();
    lastMousePosition = event->pos();
}

void ScatterPlot3DWidget::wheelEvent(QWheelEvent *event) {
    int delta = event->angleDelta().y();
    scale *= (delta > 0) ? 1.1f : 0.9f;
    update();
}

ScatterPlot3DWindow::ScatterPlot3DWindow(QWidget *parent)
    : QMainWindow(parent) {
    openGLWidget = new ScatterPlot3DWidget(this);
    setCentralWidget(openGLWidget);

    resize(1024, 768); // Задание начального размера окна
}

void ScatterPlot3DWindow::setData(const QVector<QVector3D> &vertices, const QVector<QVector<int>> &indices) {
    openGLWidget->setData(vertices, indices);
}

void ScatterPlot3DWindow::setScatteringData(const QVector<double> &absEout, const QVector<double> &normEout) {
    openGLWidget->setScatteringData(absEout, normEout);
}
