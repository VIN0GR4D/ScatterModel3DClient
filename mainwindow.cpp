#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "parser.h"
#include "raytracer.h"
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QTextEdit>
#include <QPushButton>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , openGLWidget(new OpenGLWidget(this))
    , parser(new Parser(this))
    , rayTracer(new RayTracer())
    , triangleClient(new TriangleClient(QUrl("ws://yourserveraddress:port"), this))
    , serverEnabled(false)
{
    ui->setupUi(this);
    setCentralWidget(openGLWidget);

    // Настройка компонентов пользовательского интерфейса
    QDockWidget *dockWidget = new QDockWidget("Control Panel", this);
    addDockWidget(Qt::LeftDockWidgetArea, dockWidget);
    controlWidget = new QWidget(dockWidget);
    formLayout = new QFormLayout(controlWidget);
    dockWidget->setWidget(controlWidget);

    // Элементы пользовательского интерфейса для загрузки модели
    lineEditFilePath = new QLineEdit(controlWidget);
    buttonLoadModel = new QPushButton("Загрузить объект", controlWidget);
    formLayout->addRow(new QLabel("Файл объекта:"), lineEditFilePath);
    formLayout->addRow(buttonLoadModel);

    // Элементы пользовательского интерфейса для ввода параметров
    inputWavelength = new QDoubleSpinBox(controlWidget);
    inputResolution = new QDoubleSpinBox(controlWidget);
    inputPolarization = new QComboBox(controlWidget);
    inputPortraitType = new QComboBox(controlWidget);
    buttonPerformCalculation = new QPushButton("Выполнить расчет", controlWidget);

    inputPolarization->addItems({"Горизонтальный", "Вертикальный", "Круговой"});
    inputPortraitType->addItems({"Тип 1", "Тип 2", "Тип 3"});

    formLayout->addRow(new QLabel("Длина волны (nm):"), inputWavelength);
    formLayout->addRow(new QLabel("Разрешение (m):"), inputResolution);
    formLayout->addRow(new QLabel("Поляризация:"), inputPolarization);
    formLayout->addRow(new QLabel("Портретный тип:"), inputPortraitType);
    formLayout->addRow(buttonPerformCalculation);

    // Элементы пользовательского интерфейса для отображения результатов
    resultDisplay = new QTextEdit(controlWidget);
    resultDisplay->setReadOnly(true);
    buttonSaveResults = new QPushButton("Save Results", controlWidget);
    formLayout->addRow(new QLabel("Results:"), resultDisplay);
    formLayout->addRow(buttonSaveResults);
    buttonSaveResults->hide();

    // Соединения
    connect(buttonLoadModel, &QPushButton::clicked, this, &MainWindow::loadModel);
    connect(buttonPerformCalculation, &QPushButton::clicked, this, &MainWindow::performCalculation);
    connect(triangleClient, &TriangleClient::resultsReceived, this, &MainWindow::updateResultsDisplay);
    connect(buttonSaveResults, &QPushButton::clicked, this, &MainWindow::saveResults);
    connect(parser, &Parser::fileParsed, this, [&](const QVector<QVector3D> &v, const QVector<QVector<int>> &t, const QVector<QSharedPointer<triangle>> &tri) {
        QVector3D observerPositionQVector = openGLWidget->getCameraPosition();
        rVect observerPosition = openGLWidget->QVector3DToRVect(observerPositionQVector);

        rayTracer->determineVisibility(tri, observerPosition);
        openGLWidget->setGeometry(v, t, tri);
    });
}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::loadModel() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open 3D model", "", "OBJ Files (*.obj)");
    if (!fileName.isEmpty()) {
        lineEditFilePath->setText(fileName);
        openGLWidget->clearScene();
        parser->clearData();
        parser->readFromObjFile(fileName);
    }
}

void MainWindow::performCalculation() {
    double wavelength = inputWavelength->value();
    double resolution = inputResolution->value();
    QString polarization = inputPolarization->currentText();
    QString portraitType = inputPortraitType->currentText();

    QJsonObject parameters{
        {"wavelength", wavelength},
        {"resolution", resolution},
        {"polarization", polarization},
        {"portraitType", portraitType}
    };
    buttonSaveResults->show();
    if (serverEnabled) {
        triangleClient->sendCommand(QJsonDocument(parameters).toJson(QJsonDocument::Compact));
        buttonSaveResults->show();
    } else {
        resultDisplay->setText("Server connection disabled. Unable to perform calculations.");
    }

    resultDisplay->append(QString("Prepared for calculation with wavelength %1 nm, resolution %2 m, polarization %3, portrait type %4...")
                              .arg(wavelength, 0, 'f', 2)
                              .arg(resolution, 0, 'f', 2)
                              .arg(polarization)
                              .arg(portraitType));
}

void MainWindow::updateResultsDisplay(const QString& results) {
    resultDisplay->setText(results);
}

void MainWindow::saveResults() {
    QString fileName = QFileDialog::getSaveFileName(this, "Save Results", "", "Text Files (*.txt)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << resultDisplay->toPlainText();
        }
    }
}
