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
#include "portraitwidget.h"
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QVector>
#include <QListWidget>
#include <QCheckBox>

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
    void onConnectedToServer();
    void logMessage(const QString& message);
    void updateResultsDisplay(const QJsonObject& results);
    void authorizeClient();
    void sendDataAfterAuthorization(std::function<void()> sendDataFunc);
    void openGraphWindow();
    void disconnectFromServer();
    void showPortrait();
    void toggleTheme(int state);
    void saveFile();

private:
    Ui::MainWindow *ui;
    OpenGLWidget *openGLWidget;
    Parser *parser;
    RayTracer *rayTracer;
    TriangleClient *triangleClient;

    QLineEdit *serverAddressInput;
    QPushButton *buttonSaveResults, *buttonPerformCalculation, *connectButton, *buttonApplyRotation, *buttonResetRotation;
    QTextEdit *resultDisplay, *logDisplay;
    QDoubleSpinBox *inputWavelength, *inputResolution;
    QDoubleSpinBox *inputRotationX, *inputRotationY, *inputRotationZ;
    QComboBox *inputPolarization;
    QWidget *controlWidget;
    QWidget* createThemeSwitchButton();
    QFormLayout *formLayout;
    bool serverEnabled;
    QJsonObject vectorToJson(const QSharedPointer<const rVect>& vector);
    void extractValues(const QJsonArray &array, QVector<double> &container, int depth);
    void displayResults(const QJsonObject &results);
    PortraitWidget *portraitWidget;
    double calculateAngle(int index, int totalSteps);
    double calculateAzimuth(int index, int totalSteps);
};

#endif // MAINWINDOW_H
