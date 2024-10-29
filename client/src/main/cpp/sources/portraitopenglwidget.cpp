#include "portraitopenglwidget.h"
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QVector3D>
#include <QDebug>

PortraitOpenGLWidget::PortraitOpenGLWidget(QWidget *parent)
    : QOpenGLWidget(parent)
{
}

void PortraitOpenGLWidget::setData(const QVector<QVector<double>> &absEout2D, const QVector<QVector<double>> &normEout2D)
{
    this->absEout2D = absEout2D;
    this->normEout2D = normEout2D;
    update();
}

void PortraitOpenGLWidget::clearData()
{
    absEout2D.clear();
    normEout2D.clear();
    update();
}

void PortraitOpenGLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
}

void PortraitOpenGLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
    QMatrix4x4 projection;
    projection.perspective(45.0f, float(w) / h, 0.1f, 100.0f);
    projection.translate(0.0f, 0.0f, -10.0f);
    projection.rotate(25.0f, 1.0f, 0.0f, 0.0f);
    projection.rotate(-45.0f, 0.0f, 1.0f, 0.0f);
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf(projection.constData());
}

void PortraitOpenGLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (absEout2D.isEmpty())
        return;

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glBegin(GL_POINTS);
    for (int x = 0; x < absEout2D.size(); ++x) {
        for (int y = 0; y < absEout2D[x].size(); ++y) {
            float z = absEout2D[x][y];
            glVertex3f(x, y, z);
        }
    }
    glEnd();
}
