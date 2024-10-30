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
#include <QFutureWatcher>

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

public slots:
    void updateRotationX(double x) {
        inputRotationX->blockSignals(true);
        inputRotationX->setValue(fmod(x, 360.0));  // Применяем модуль от 360
        inputRotationX->blockSignals(false);
    }

    void updateRotationY(double y) {
        inputRotationY->blockSignals(true);
        inputRotationY->setValue(fmod(y, 360.0));  // Применяем модуль от 360
        inputRotationY->blockSignals(false);
    }

    void updateRotationZ(double z) {
        inputRotationZ->blockSignals(true);
        inputRotationZ->setValue(fmod(z, 360.0));  // Применяем модуль от 360
        inputRotationZ->blockSignals(false);
    }


private slots:
    void loadModel();
    void applyRotation();
    void resetRotation();
    void performCalculation();
    void saveResults();
    void connectToServer();
    void onConnectedToServer();
    void logMessage(const QString& message) const;
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
    void openScatterPlot3DWindow();
    void saveLog();
    void showAboutDialog();
    void displayResults(const QJsonObject &results);
    void processResults(const QJsonObject &results, bool angleChecked, bool azimuthChecked, bool rangeChecked);

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

    QVector<double> absEout;
    QVector<double> normEout;
    QVector<QVector<double>> absEout2D;
    QVector<QVector<double>> normEout2D;
    QCheckBox *anglePortraitCheckBox;
    QCheckBox *azimuthPortraitCheckBox;
    QCheckBox *rangePortraitCheckBox;
    QString storedResults;
    QComboBox *freqBandComboBox;
    QCheckBox *pplaneCheckBox;

    QComboBox *radiationPolarizationComboBox;
    QComboBox *receivePolarizationComboBox;

    QJsonObject vectorToJson(const QSharedPointer<const rVect>& vector);
    void extractValues(const QJsonArray &array, QVector<double> &container, int depth);
    void extract2DValues(const QJsonArray &array, QVector<QVector<double>> &container);
    Graph3DWindow *graph3DWindow;
    PortraitWindow *portraitWindow;
    bool isDarkTheme;
    int freqBand;
    QJsonObject getScatteringDataFromServer();
    rVect calculateDirectVectorFromRotation();

    QFutureWatcher<void> *resultsWatcher;
};

#endif // MAINWINDOW_H
