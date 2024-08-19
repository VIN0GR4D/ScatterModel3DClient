#ifndef GRAPH3DWINDOW_H
#define GRAPH3DWINDOW_H

#include <Qt3DCore/QEntity>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QSphereMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <QMainWindow>
#include <QVector>
#include <QSlider>
#include <QPushButton>

class Graph3DWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit Graph3DWindow(QWidget *parent = nullptr);
    void setData(const QVector<QVector<double>> &absEout, const QVector<QVector<double>> &normEout);
    void clearData();

private slots:
    void onZoomChanged(int value);
    void onMoveUp();
    void onMoveDown();
    void onMoveLeft();
    void onMoveRight();
    void onRotateLeft();
    void onRotateRight();

private:
    Qt3DExtras::Qt3DWindow *view;
    Qt3DCore::QEntity *rootEntity;
    Qt3DRender::QCamera *cameraEntity;
    QVector<Qt3DCore::QEntity*> sphereEntities;

    Qt3DExtras::QSphereMesh *sharedSphereMesh;
    Qt3DExtras::QPhongMaterial *sharedMaterial;

    QSlider *zoomSlider;
    QPushButton *moveUpButton;
    QPushButton *moveDownButton;
    QPushButton *moveLeftButton;
    QPushButton *moveRightButton;
    QPushButton *rotateLeftButton;
    QPushButton *rotateRightButton;

    void create3DGraph(const QVector<QVector<double>> &data);
    void setupUI();
};

#endif // GRAPH3DWINDOW_H
