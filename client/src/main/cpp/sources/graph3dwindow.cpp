#include "graph3dwindow.h"
#include <Qt3DCore/QEntity>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DCore/QTransform>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DRender/QCamera>
#include <Qt3DRender/QCameraLens>
#include <Qt3DRender/QRenderSettings>
#include <Qt3DExtras/QForwardRenderer>
#include <Qt3DExtras/QOrbitCameraController>
#include <QVBoxLayout>
#include <QWidget>
#include <QColor>
#include <Qt3DRender/QDirectionalLight>
#include <QSlider>
#include <QPushButton>
#include <QHBoxLayout>
#include <QDockWidget>

Graph3DWindow::Graph3DWindow(QWidget *parent) :
    QMainWindow(parent),
    view(new Qt3DExtras::Qt3DWindow()),
    rootEntity(new Qt3DCore::QEntity()),
    sharedSphereMesh(new Qt3DExtras::QSphereMesh()),
    sharedMaterial(new Qt3DExtras::QPhongMaterial())
{
    // Настройка цвета фона
    view->defaultFrameGraph()->setClearColor(QColor(QRgb(0x2e2e2e))); // Темный фон для контраста

    QWidget *container = QWidget::createWindowContainer(view);
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->addWidget(container);
    setCentralWidget(centralWidget);

    view->setRootEntity(rootEntity);

    setWindowTitle("3D Graph");
    resize(800, 600);

    // Настройка камеры
    cameraEntity = view->camera();
    cameraEntity->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    cameraEntity->setPosition(QVector3D(0, 0, 20));
    cameraEntity->setViewCenter(QVector3D(0, 0, 0));

    // Настройка света
    Qt3DCore::QEntity *lightEntity = new Qt3DCore::QEntity(rootEntity);
    Qt3DRender::QDirectionalLight *light = new Qt3DRender::QDirectionalLight(lightEntity);
    light->setWorldDirection(QVector3D(0.0f, -1.0f, -1.0f));
    lightEntity->addComponent(light);

    // Настройка контроллера камеры
    Qt3DExtras::QOrbitCameraController *camController = new Qt3DExtras::QOrbitCameraController(rootEntity);
    camController->setCamera(cameraEntity);

    // Настройка кэшированного меша и материала
    sharedSphereMesh->setRadius(0.1f);
    sharedMaterial->setDiffuse(QColor::fromRgb(150, 150, 150));

    // Настройка UI для управления
    setupUI();
}

void Graph3DWindow::setupUI() {
    zoomSlider = new QSlider(Qt::Horizontal, this);
    zoomSlider->setRange(10, 100); // Диапазон изменения масштаба
    zoomSlider->setValue(45);
    connect(zoomSlider, &QSlider::valueChanged, this, &Graph3DWindow::onZoomChanged);

    moveUpButton = new QPushButton("Up", this);
    connect(moveUpButton, &QPushButton::clicked, this, &Graph3DWindow::onMoveUp);

    moveDownButton = new QPushButton("Down", this);
    connect(moveDownButton, &QPushButton::clicked, this, &Graph3DWindow::onMoveDown);

    moveLeftButton = new QPushButton("Left", this);
    connect(moveLeftButton, &QPushButton::clicked, this, &Graph3DWindow::onMoveLeft);

    moveRightButton = new QPushButton("Right", this);
    connect(moveRightButton, &QPushButton::clicked, this, &Graph3DWindow::onMoveRight);

    rotateLeftButton = new QPushButton("Rotate Left", this);
    connect(rotateLeftButton, &QPushButton::clicked, this, &Graph3DWindow::onRotateLeft);

    rotateRightButton = new QPushButton("Rotate Right", this);
    connect(rotateRightButton, &QPushButton::clicked, this, &Graph3DWindow::onRotateRight);

    // Используем боковую панель (DockWidget) для управления
    QDockWidget *controlDock = new QDockWidget("Controls", this);
    QWidget *controlsWidget = new QWidget(this);
    QVBoxLayout *controlsLayout = new QVBoxLayout(controlsWidget);
    controlsLayout->addWidget(zoomSlider);
    controlsLayout->addWidget(moveUpButton);
    controlsLayout->addWidget(moveDownButton);
    controlsLayout->addWidget(moveLeftButton);
    controlsLayout->addWidget(moveRightButton);
    controlsLayout->addWidget(rotateLeftButton);
    controlsLayout->addWidget(rotateRightButton);
    controlsWidget->setLayout(controlsLayout);
    controlDock->setWidget(controlsWidget);

    // Добавляем возможность скрывать панель управления
    addDockWidget(Qt::RightDockWidgetArea, controlDock);
    controlDock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
}

void Graph3DWindow::onZoomChanged(int value) {
    float zoomFactor = static_cast<float>(value);
    cameraEntity->lens()->setPerspectiveProjection(zoomFactor, 16.0f/9.0f, 0.1f, 1000.0f);
}

void Graph3DWindow::onMoveUp() {
    QVector3D position = cameraEntity->position();
    position.setY(position.y() + 1.0f);
    cameraEntity->setPosition(position);
}

void Graph3DWindow::onMoveDown() {
    QVector3D position = cameraEntity->position();
    position.setY(position.y() - 1.0f);
    cameraEntity->setPosition(position);
}

void Graph3DWindow::onMoveLeft() {
    QVector3D position = cameraEntity->position();
    position.setX(position.x() - 1.0f);
    cameraEntity->setPosition(position);
}

void Graph3DWindow::onMoveRight() {
    QVector3D position = cameraEntity->position();
    position.setX(position.x() + 1.0f);
    cameraEntity->setPosition(position);
}

void Graph3DWindow::onRotateLeft() {
    QVector3D viewCenter = cameraEntity->viewCenter();
    QVector3D cameraPosition = cameraEntity->position();
    QMatrix4x4 rotationMatrix;
    rotationMatrix.rotate(-10.0f, QVector3D(0, 1, 0));
    cameraPosition = rotationMatrix.map(cameraPosition);
    cameraEntity->setPosition(cameraPosition);
    cameraEntity->setViewCenter(viewCenter);
}

void Graph3DWindow::onRotateRight() {
    QVector3D viewCenter = cameraEntity->viewCenter();
    QVector3D cameraPosition = cameraEntity->position();
    QMatrix4x4 rotationMatrix;
    rotationMatrix.rotate(10.0f, QVector3D(0, 1, 0));
    cameraPosition = rotationMatrix.map(cameraPosition);
    cameraEntity->setPosition(cameraPosition);
    cameraEntity->setViewCenter(viewCenter);
}

void Graph3DWindow::setData(const QVector<QVector<double>> &absEout, const QVector<QVector<double>> &normEout) {
    clearData();
    create3DGraph(absEout);
}

void Graph3DWindow::clearData() {
    for (Qt3DCore::QEntity *entity : sphereEntities) {
        delete entity;
    }
    sphereEntities.clear();
}

void Graph3DWindow::create3DGraph(const QVector<QVector<double>> &data) {
    for (int x = 0; x < data.size(); ++x) {
        for (int y = 0; y < data[x].size(); ++y) {
            float z = data[x][y];

            Qt3DCore::QEntity *sphereEntity = new Qt3DCore::QEntity(rootEntity);

            Qt3DCore::QTransform *sphereTransform = new Qt3DCore::QTransform();
            sphereTransform->setTranslation(QVector3D(x, y, z));

            Qt3DExtras::QPhongMaterial *sphereMaterial = new Qt3DExtras::QPhongMaterial();
            sphereMaterial->setDiffuse(QColor::fromRgbF(x / float(data.size()), y / float(data[x].size()), z / 5.0f));

            sphereEntity->addComponent(sharedSphereMesh);
            sphereEntity->addComponent(sphereTransform);
            sphereEntity->addComponent(sphereMaterial);

            sphereEntities.append(sphereEntity);
        }
    }
}
