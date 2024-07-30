#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "openglwidget.h"
#include "parser.h"
#include "raytracer.h"
#include "triangleclient.h"
#include "portraitwindow.h"
#include "graph3dwindow.h"
#include <QMainWindow>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QFileDialog>
#include <QFormLayout>
#include <QDockWidget>
#include <QTextEdit>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QVector>
#include <QListWidget>
#include <QCheckBox>
#include <QJsonObject>
#include <QJsonArray>
#include <QRadioButton>

class OpenGLWidget;
class Parser;
class RayTracer;
class TriangleClient;
class PortraitWindow;

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
    void saveResults();
    void connectToServer();
    void onConnectedToServer();
    void logMessage(const QString& message);
    void authorizeClient();
    void sendDataAfterAuthorization(std::function<void()> sendDataFunc);
    void openGraphWindow();
    void showGraph3D();
    void disconnectFromServer();
    void showPortrait();
    void toggleTheme();
    void saveFile();
    void loadTheme(const QString &themePath, const QString &iconPath, QAction *action);
    void openResultsWindow();
    void openHeatmapWindow();
    void openScatterPlot3DWindow();
    void saveLog();
    void showAboutDialog();

private:
    Ui::MainWindow *ui;
    OpenGLWidget *openGLWidget;
    Parser *parser;
    std::unique_ptr<RayTracer> rayTracer;
    TriangleClient *triangleClient;
    QLineEdit *serverAddressInput;
    QPushButton *connectButton, *buttonApplyRotation, *buttonResetRotation;
    QTextEdit *logDisplay;
    QDoubleSpinBox *inputWavelength, *inputResolution;
    QDoubleSpinBox *inputRotationX, *inputRotationY, *inputRotationZ;
    QComboBox *inputPolarizationRadiation, *inputPolarizationReceive;
    QWidget *controlWidget;
    QFormLayout *formLayout;
    bool serverEnabled;
    QJsonObject vectorToJson(const QSharedPointer<const rVect>& vector);
    void extractValues(const QJsonArray &array, QVector<double> &container, int depth);
    void extract2DValues(const QJsonArray &array, QVector<QVector<double>> &container);
    void displayResults(const QJsonObject &results);
    Graph3DWindow *graph3DWindow;
    PortraitWindow *portraitWindow;
    double calculateAngle(int index, int totalSteps);
    double calculateAzimuth(int index, int totalSteps);
    bool isDarkTheme;
    int freqBand;
    QJsonObject getScatteringDataFromServer();
};

#endif // MAINWINDOW_H
