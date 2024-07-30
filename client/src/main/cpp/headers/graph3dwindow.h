#ifndef GRAPH3DWINDOW_H
#define GRAPH3DWINDOW_H

#include <Qt3DCore/QEntity>
#include <Qt3DExtras/Qt3DWindow>
#include <QMainWindow>
#include <QVector>

class Graph3DWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit Graph3DWindow(QWidget *parent = nullptr);
    void setData(const QVector<QVector<double>> &absEout, const QVector<QVector<double>> &normEout);
    void clearData();

private:
    Qt3DExtras::Qt3DWindow *view;
    Qt3DCore::QEntity *rootEntity;
    QVector<Qt3DCore::QEntity*> sphereEntities;

    void create3DGraph(const QVector<QVector<double>> &data);
};

#endif // GRAPH3DWINDOW_H
