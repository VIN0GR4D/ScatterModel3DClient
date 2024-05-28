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
#include <QProgressDialog>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , openGLWidget(new OpenGLWidget(this))
    , parser(new Parser(this))
    , rayTracer(new RayTracer())
    , triangleClient(nullptr)
    , serverEnabled(false) // Отключаем сервер
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
    inputPortraitType->addItems({"Угломестный", "Азимутальный", "Дальностный"});

    formLayout->addRow(new QLabel("Длина волны (nm):"), inputWavelength);
    formLayout->addRow(new QLabel("Разрешение (m):"), inputResolution);
    formLayout->addRow(new QLabel("Поляризация:"), inputPolarization);
    formLayout->addRow(new QLabel("Портретный тип:"), inputPortraitType);
    formLayout->addRow(buttonPerformCalculation);

    // Элементы для подключения к серверу
    serverAddressInput = new QLineEdit(controlWidget);
    serverAddressInput->setPlaceholderText("ws://yourserveraddress:port");
    connectButton = new QPushButton("Connect", controlWidget);
    formLayout->addRow(new QLabel("Server Address:"), serverAddressInput);
    formLayout->addRow(connectButton);

    logDisplay = new QTextEdit(controlWidget);
    logDisplay->setReadOnly(true);
    formLayout->addRow(new QLabel("Log:"), logDisplay);

    // Элементы пользовательского интерфейса для отображения результатов
    resultDisplay = new QTextEdit(controlWidget);
    resultDisplay->setReadOnly(true);
    buttonSaveResults = new QPushButton("Save Results", controlWidget);
    formLayout->addRow(new QLabel("Results:"), resultDisplay);
    formLayout->addRow(buttonSaveResults);
    buttonSaveResults->hide();

    // Элементы для ввода углов поворота
    inputRotationX = new QDoubleSpinBox(controlWidget);
    inputRotationX->setRange(-360, 360);
    inputRotationX->setSuffix(" °");

    inputRotationY = new QDoubleSpinBox(controlWidget);
    inputRotationY->setRange(-360, 360);
    inputRotationY->setSuffix(" °");

    inputRotationZ = new QDoubleSpinBox(controlWidget);
    inputRotationZ->setRange(-360, 360);
    inputRotationZ->setSuffix(" °");

    buttonApplyRotation = new QPushButton("Применить поворот", controlWidget);
    buttonResetRotation = new QPushButton("Сбросить поворот", controlWidget);

    formLayout->addRow(new QLabel("Поворот по X:"), inputRotationX);
    formLayout->addRow(new QLabel("Поворот по Y:"), inputRotationY);
    formLayout->addRow(new QLabel("Поворот по Z:"), inputRotationZ);
    formLayout->addRow(buttonApplyRotation);
    formLayout->addRow(buttonResetRotation);

    // Соединения
    connect(buttonApplyRotation, &QPushButton::clicked, this, &MainWindow::applyRotation);
    connect(buttonResetRotation, &QPushButton::clicked, this, &MainWindow::resetRotation);
    connect(buttonLoadModel, &QPushButton::clicked, this, &MainWindow::loadModel);
    connect(buttonPerformCalculation, &QPushButton::clicked, this, &MainWindow::performCalculation);
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::connectToServer);
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

// Функция для загрузки модели
void MainWindow::loadModel() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open 3D model", "", "OBJ Files (*.obj)");
    if (!fileName.isEmpty()) {
        lineEditFilePath->setText(fileName);
        openGLWidget->clearScene();
        parser->clearData();
        parser->readFromObjFile(fileName);
    }
}

void MainWindow::applyRotation() {
    float rotationX = inputRotationX->value();
    float rotationY = inputRotationY->value();
    float rotationZ = inputRotationZ->value();

    openGLWidget->setRotation(rotationX, rotationY, rotationZ);
}

void MainWindow::resetRotation() {
    inputRotationX->setValue(0);
    inputRotationY->setValue(0);
    inputRotationZ->setValue(0);

    openGLWidget->setRotation(0, 0, 0);
}

// Функция для выполнения расчета
void MainWindow::performCalculation() {
    if (!triangleClient || !triangleClient->isConnected()) {
        resultDisplay->setText("Not connected to the server. Please connect first.");
        return;
    }

    double wavelength = inputWavelength->value();
    double resolution = inputResolution->value();
    QString polarizationText = inputPolarization->currentText();
    QString portraitTypeText = inputPortraitType->currentText();

    // Преобразование текста поляризации в соответствующие значения
    int polarRadiation = 0; // Вертикальная поляризация по умолчанию
    int polarRecive = 0;    // Вертикальная поляризация по умолчанию
    if (polarizationText == "Горизонтальный") {
        polarRadiation = 1;
        polarRecive = 1;
    } else if (polarizationText == "Круговой") {
        polarRadiation = 2;
        polarRecive = 2;
    }

    // Определение типов радиопортретов
    bool typeAngle = (portraitTypeText == "Угломестный");
    bool typeAzimut = (portraitTypeText == "Азимутальный");
    bool typeLength = (portraitTypeText == "Дальностный");

    triangleClient->setPolarizationAndType(polarRadiation, polarRecive, typeAngle, typeAzimut, typeLength);

    QJsonObject parameters{
        {"wavelength", wavelength},
        {"resolution", resolution},
        {"polarRadiation", polarRadiation},
        {"polarRecive", polarRecive},
        {"typeAngle", typeAngle},
        {"typeAzimut", typeAzimut},
        {"typeLength", typeLength}
    };

    triangleClient->sendCommand(QJsonDocument(parameters).toJson(QJsonDocument::Compact));
    buttonSaveResults->show();

    resultDisplay->append(QString("Prepared for calculation with wavelength %1 nm, resolution %2 m, polarization %3, portrait type %4...")
                              .arg(wavelength, 0, 'f', 2)
                              .arg(resolution, 0, 'f', 2)
                              .arg(polarizationText)
                              .arg(portraitTypeText));
}

// Функция для обновления отображения результатов
void MainWindow::updateResultsDisplay(const QString& results) {
    resultDisplay->setText(results);
}

// Функция для подключения к серверу
void MainWindow::connectToServer() {
    QString serverAddress = serverAddressInput->text().trimmed();
    if (!serverAddress.isEmpty()) {
        if (triangleClient) {
            triangleClient->deleteLater();
        }
        triangleClient = new TriangleClient(QUrl(serverAddress), this);

        // Подключение сигналов
        connect(triangleClient, &TriangleClient::resultsReceived, this, &MainWindow::updateResultsDisplay);
        connect(triangleClient, &TriangleClient::logMessage, this, &MainWindow::logMessage);

        serverEnabled = true;
        logMessage("Connecting to server: " + serverAddress);
    } else {
        logMessage("Server address is empty.");
    }
}

void MainWindow::logMessage(const QString& message) {
    logDisplay->append(message);
}

// Функция для сохранения результатов
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
