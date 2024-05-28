#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QFileDialog>
#include <QFormLayout>
#include <QDockWidget>
#include <QTextEdit>
#include "openglwidget.h"
#include "parser.h"
#include "raytracer.h"
#include "triangleclient.h"
#include <QDoubleSpinBox>
#include <QComboBox>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void loadModel();
    void applyRotation();
    void resetRotation();
    void performCalculation();
    void updateResultsDisplay(const QString& results);
    void saveResults();
    void connectToServer();
    void logMessage(const QString& message);

private:
    Ui::MainWindow *ui;
    OpenGLWidget *openGLWidget;
    Parser *parser;
    RayTracer *rayTracer;
    TriangleClient *triangleClient;

    QLineEdit *lineEditFilePath, *serverAddressInput;
    QPushButton *buttonLoadModel, *buttonSaveResults, *buttonPerformCalculation, *connectButton, *buttonApplyRotation, *buttonResetRotation;
    QTextEdit *resultDisplay, *logDisplay;
    QDoubleSpinBox *inputWavelength, *inputResolution;
    QDoubleSpinBox *inputRotationX, *inputRotationY, *inputRotationZ;
    QComboBox *inputPolarization, *inputPortraitType;
    QWidget *controlWidget;
    QFormLayout *formLayout;
    bool serverEnabled;
};

#endif // MAINWINDOW_H
