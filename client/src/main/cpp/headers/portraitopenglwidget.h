#ifndef PORTRAITOPENGLWIDGET_H
#define PORTRAITOPENGLWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QVector>

class PortraitOpenGLWidget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit PortraitOpenGLWidget(QWidget *parent = nullptr);
    void setData(const QVector<QVector<double>> &absEout2D, const QVector<QVector<double>> &normEout2D);
    void clearData();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    QVector<QVector<double>> absEout2D;
    QVector<QVector<double>> normEout2D;
};

#endif // PORTRAITOPENGLWIDGET_H
