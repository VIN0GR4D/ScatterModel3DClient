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

Graph3DWindow::Graph3DWindow(QWidget *parent) :
    QMainWindow(parent),
    view(new Qt3DExtras::Qt3DWindow()),
    rootEntity(new Qt3DCore::QEntity())
{
    view->defaultFrameGraph()->setClearColor(QColor(QRgb(0x4d4d4f)));
    QWidget *container = QWidget::createWindowContainer(view);
    QWidget *centralWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(centralWidget);
    layout->addWidget(container);
    setCentralWidget(centralWidget);

    view->setRootEntity(rootEntity);

    setWindowTitle("3D Graph");
    resize(800, 600);

    // Setting up the camera
    Qt3DRender::QCamera *cameraEntity = view->camera();
    cameraEntity->lens()->setPerspectiveProjection(45.0f, 16.0f/9.0f, 0.1f, 1000.0f);
    cameraEntity->setPosition(QVector3D(0, 0, 20));
    cameraEntity->setViewCenter(QVector3D(0, 0, 0));

    // Setting up the light
    Qt3DCore::QEntity *lightEntity = new Qt3DCore::QEntity(rootEntity);
    Qt3DRender::QDirectionalLight *light = new Qt3DRender::QDirectionalLight(lightEntity);
    light->setWorldDirection(QVector3D(0.0f, -1.0f, -1.0f));
    lightEntity->addComponent(light);

    // Setting up the camera controller
    Qt3DExtras::QOrbitCameraController *camController = new Qt3DExtras::QOrbitCameraController(rootEntity);
    camController->setCamera(cameraEntity);
}

void Graph3DWindow::setData(const QVector<QVector<double>> &absEout, const QVector<QVector<double>> &normEout) {
    clearData(); // Очистим предыдущие данные
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
            Qt3DExtras::QSphereMesh *sphereMesh = new Qt3DExtras::QSphereMesh();
            sphereMesh->setRadius(0.1f);

            Qt3DCore::QTransform *sphereTransform = new Qt3DCore::QTransform();
            sphereTransform->setTranslation(QVector3D(x, y, z));

            Qt3DExtras::QPhongMaterial *sphereMaterial = new Qt3DExtras::QPhongMaterial();
            sphereMaterial->setDiffuse(QColor::fromRgbF(x / float(data.size()), y / float(data[x].size()), z / 5.0f));

            sphereEntity->addComponent(sphereMesh);
            sphereEntity->addComponent(sphereTransform);
            sphereEntity->addComponent(sphereMaterial);

            sphereEntities.append(sphereEntity);
        }
    }
}
