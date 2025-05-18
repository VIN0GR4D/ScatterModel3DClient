#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "openglwidget.h"
#include "parser.h"
#include "raytracer.h"
#include "portraitwindow.h"
#include "projectserializer.h"
#include "meshfilter.h"
#include "notification.h"
#include "notificationmanager.h"
#include "patterndiagramwindow.h"
#include "connectionmanager.h"
#include "modelcontroller.h"
#include "portraittypes.h"
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
#include <QAction>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QStackedWidget>
#include <QToolBar>
#include <QToolButton>
#include <QActionGroup>
#include <QProgressBar>

class OpenGLWidget;
class Parser;
class RayTracer;
class TriangleClient;
class PortraitWindow;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct PortraitData {
    PortraitDimension dimension;
    QVector<QVector<QVector<double>>> data3D;
    QVector<QVector<double>> data2D;
    QVector<double> data1D;
};

class MainWindow : public QMainWindow, public IConnectionObserver, public IModelObserver
{
    Q_OBJECT

public:
    MainWindow(ConnectionManager* connectionManager, QWidget *parent = nullptr);
    ~MainWindow();

    // Реализация интерфейса IConnectionObserver
    void onConnectionStatusChanged(bool connected) override;
    void onResultsReceived(const QJsonObject &results) override;
    void onCalculationProgress(int progress) override;
    void onLogMessage(const QString &message) override;
    void onNotification(const QString &message, Notification::Type type) override;
    void onCalculationAborted() override;

    // Реализация интерфейса IModelObserver
    void onModelLoaded(const QString &fileName, int nodesCount, int trianglesCount) override;
    void onModelModified() override;
    void onModelClosed() override;
    void onFilteringCompleted(const MeshFilter::FilterStats &stats) override;

protected:
    // Переопределение события закрытия окна
    void closeEvent(QCloseEvent *event) override;

public slots:
    void updateRotationX(double x);
    void updateRotationY(double y);
    void updateRotationZ(double z);

private slots:
    void performCalculation();
    void saveResults();
    void logMessage(const QString& message);
    void openGraphWindow();
    void showPortrait();
    void toggleTheme();
    bool saveProject();
    void loadTheme(const QString &themePath, const QString &iconPath, QAction *action);
    void openResultsWindow();
    void updateMenuActions();
    void onPortraitTypeChanged();
    void saveLog();
    void showAboutDialog();
    void displayResults(const QJsonObject &results);
    void processResults(const QJsonObject &results, bool angleChecked, bool azimuthChecked, bool rangeChecked);
    void onGridCheckBoxStateChanged(int state);
    void openProject();
    void newProject();
    void abortCalculation();
    void updateCalculationProgress(int progress);
    void showPatternDiagram();

    // Слоты для работы с ModelController
    void handleLoadModel();
    void handleApplyRotation();
    void handleResetRotation();
    void handleFilterModel();
    void handleToggleShadowTriangles();
    void handleCloseModel();

    void handleResetScale();

signals:
    void connectionStatusChanged(bool connected);

private:
    PortraitData portraitData;
    PortraitDimension detectPortraitDimension(const QJsonArray &array);

    void extract3DData(const QJsonArray &array, QVector<QVector<QVector<double>>> &container);
    void prepare2DDataForDisplay(const PortraitData &data,
                                 bool angleChecked, bool azimuthChecked, bool rangeChecked,
                                 QVector<double> &data1D, QVector<QVector<double>> &data2D);

    Ui::MainWindow *ui;
    OpenGLWidget *openGLWidget;
    QLineEdit *serverAddressInput;
    QPushButton *connectButton, *buttonApplyRotation, *buttonResetRotation;
    QPushButton *buttonResetScale;
    QTextEdit *logDisplay;
    QDoubleSpinBox *inputWavelength, *inputResolution;
    QDoubleSpinBox *inputRotationX, *inputRotationY, *inputRotationZ;
    QComboBox *inputPolarizationRadiation, *inputPolarizationReceive;
    // Действия меню "Результаты"
    QAction *openResultsAction;
    QAction *openGraphAction;
    QAction *showPortraitAction;

    // Хранение состояния данных
    bool hasNumericalData;
    bool hasGraphData;

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
    QCheckBox *gridCheckBox;
    PortraitWindow *portraitWindow;
    ProjectData currentProjectData;
    bool isModified;
    void setModified(bool modified = true);
    bool maybeSave();
    bool isDarkTheme;
    int freqBand;
    QJsonObject getScatteringDataFromServer();
    rVect calculateDirectVectorFromRotation();
    QFutureWatcher<void> *resultsWatcher;

    QTreeWidget* filterTreeWidget;
    void updateFilterStats(const MeshFilter::FilterStats& stats);
    QTreeWidgetItem* shellStatsItem;
    QTreeWidgetItem* visibilityStatsItem;

    QStackedWidget *stackedWidget;
    QWidget *parametersWidget;    // Для параметров
    QWidget *filteringWidget;     // Для фильтрации
    QWidget *serverWidget;        // Для сервера
    QLabel* totalTrianglesLabel; // Общее количество треугольников
    QLabel* totalNodesLabel; // Общее количество вершин
    QLabel* modelFileNameLabel; // Название файла объекта
    QLabel* shellCountDisplay;  // Отображение количества треугольников в оболоче объекта
    QLabel* visibleCountDisplay;  // Отображение количества видимых треугольников объекта
    QLabel* removedShellDisplay;   // Отображение количества удаленных треугольников при оптимизации
    QLabel* removedShadowDisplay;  // Отображение количества скрытых теневых треугольников

    void createToolBar();
    void setupParametersWidget();
    void setupFilteringWidget();
    void setupServerWidget();
    QDialog* logWindow = nullptr;
    void showLogWindow();
    void showNotification(const QString &message, Notification::Type type);
    void updateConnectionStatus(QLabel* connectionLabel, QLabel* authLabel);
    QPushButton* abortCalculationButton;
    QProgressBar* progressBar;

    PatternDiagramWindow *patternDiagramWindow;

    ConnectionManager* m_connectionManager;

    std::unique_ptr<ModelController> m_modelController;
};

#endif // MAINWINDOW_H
