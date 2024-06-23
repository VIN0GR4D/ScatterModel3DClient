#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "parser.h"
#include "raytracer.h"
#include "qcustomplot.h"
#include "graphwindow.h"
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

QVector<double> absEout;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , openGLWidget(new OpenGLWidget(this))
    , parser(new Parser(this))
    , rayTracer(new RayTracer())
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

    QPushButton *buttonOpenGraphWindow = new QPushButton("Открыть окно графика", controlWidget);
    formLayout->addRow(buttonOpenGraphWindow);
    connect(buttonOpenGraphWindow, &QPushButton::clicked, this, &MainWindow::openGraphWindow);

    this->resize(1280, 720);

    // Соединения
    connect(buttonApplyRotation, &QPushButton::clicked, this, &MainWindow::applyRotation);
    connect(buttonResetRotation, &QPushButton::clicked, this, &MainWindow::resetRotation);
    connect(buttonLoadModel, &QPushButton::clicked, this, &MainWindow::loadModel);
    connect(buttonPerformCalculation, &QPushButton::clicked, this, &MainWindow::performCalculation);
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::connectToServer);
    connect(buttonSaveResults, &QPushButton::clicked, this, &MainWindow::saveResults);
    connect(triangleClient, &TriangleClient::resultsReceived, this, QOverload<const QJsonObject &>::of(&MainWindow::displayResults));
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

void MainWindow::displayResults(const QJsonObject &results) {
    QJsonDocument doc(results);
    QString resultsString = doc.toJson(QJsonDocument::Indented);
    resultDisplay->setText(resultsString);

    // Извлечение данных absEout из JSON-объекта
    QJsonArray absEoutArray = results["absEout"].toArray();
    absEout.clear();

    for (const QJsonValue &value : absEoutArray) {
        QJsonArray innerArray = value.toArray();
        if (!innerArray.isEmpty()) {
            double val = innerArray[0].toArray()[0].toDouble(); // Изменено здесь
            absEout.append(val);
        } else {
            qDebug() << "Inner array is empty or not found";
        }
    }

    // Выводим данные для отладки
    qDebug() << "absEout extracted from JSON:" << absEout;
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

// Метод для авторизации
void MainWindow::authorizeClient() {
    QString username = "user";  // Логин
    QString password = "user";  // Пароль
    triangleClient->authorize(username, password);
}

void MainWindow::sendDataAfterAuthorization(std::function<void()> sendDataFunc) {
    if (!triangleClient || !triangleClient->isConnected()) {
        resultDisplay->setText("Not connected to the server. Please connect first.");
        return;
    }

    if (!triangleClient->isAuthorized()) {
        authorizeClient();  // Вызываем метод авторизации, если не авторизованы
        QTimer::singleShot(2000, this, [this, sendDataFunc]() {
            if (triangleClient->isConnected() && triangleClient->isAuthorized()) {
                sendDataFunc();  // Отправляем данные после успешной авторизации
            } else {
                resultDisplay->setText("Authorization failed or connection lost.");
            }
        });
    } else {
        sendDataFunc();  // Если уже авторизованы, отправляем данные сразу
    }
}

QJsonObject MainWindow::vectorToJson(const QSharedPointer<const rVect>& vector) {
    QJsonObject obj;
    obj["x"] = vector->getX();
    obj["y"] = vector->getY();
    obj["z"] = vector->getZ();
    return obj;
}

// Функция для выполнения расчета
void MainWindow::performCalculation() {
    double wavelength = inputWavelength->value();
    double resolution = inputResolution->value();
    QString polarizationText = inputPolarization->currentText();
    QString portraitTypeText = inputPortraitType->currentText();

    int polarRadiation = 0; // Вертикальная поляризация по умолчанию
    int polarRecive = 0;    // Вертикальная поляризация по умолчанию
    if (polarizationText == "Горизонтальный") {
        polarRadiation = 1;
        polarRecive = 1;
    } else if (polarizationText == "Круговой") {
        polarRadiation = 2;
        polarRecive = 2;
    }

    bool typeAngle = (portraitTypeText == "Угломестный");
    bool typeAzimut = (portraitTypeText == "Азимутальный");
    bool typeLength = (portraitTypeText == "Дальностный");

    QVector<QSharedPointer<triangle>> triangles = openGLWidget->getTriangles();
    QJsonObject dataObject;
    QJsonArray visibleTrianglesArray;

    int index = 0;
    for (const auto& tri : triangles) {
        QSharedPointer<rVect> v1 = tri->getV1();
        QSharedPointer<rVect> v2 = tri->getV2();
        QSharedPointer<rVect> v3 = tri->getV3();

        dataObject[QString::number(index++)] = v1->getX();
        dataObject[QString::number(index++)] = v1->getY();
        dataObject[QString::number(index++)] = v1->getZ();
        dataObject[QString::number(index++)] = v2->getX();
        dataObject[QString::number(index++)] = v2->getY();
        dataObject[QString::number(index++)] = v2->getZ();
        dataObject[QString::number(index++)] = v3->getX();
        dataObject[QString::number(index++)] = v3->getY();
        dataObject[QString::number(index++)] = v3->getZ();

        visibleTrianglesArray.append(tri->getVisible());
    }

    QVector3D cameraPosition = openGLWidget->getCameraPosition();
    rVect directVector = openGLWidget->QVector3DToRVect(cameraPosition);

    QJsonObject modelData;
    modelData["data"] = dataObject;
    modelData["visbleTriangles"] = visibleTrianglesArray; // исправлено: "visbleTriangles" вместо "visibleTriangles"
    modelData["freqBand"] = 5;  // Пример диапазона частот
    modelData["polarRadiation"] = polarRadiation;
    modelData["polarRecive"] = polarRecive;
    modelData["typeAngle"] = typeAngle;
    modelData["typeAzimut"] = typeAzimut;
    modelData["typeLength"] = typeLength;
    modelData["pplane"] = false;
    modelData["directVector"] = vectorToJson(QSharedPointer<rVect>::create(directVector));
    modelData["resolution"] = resolution;
    modelData["wavelength"] = wavelength;

    // Добавим отладочное сообщение для проверки данных перед отправкой
    QJsonDocument doc(modelData);
    qDebug() << "Model data to be sent to server:" << doc.toJson(QJsonDocument::Indented);

    sendDataAfterAuthorization([this, modelData]() {
        triangleClient->sendModelData(modelData);
    });

    buttonSaveResults->show();
}

// Функция для обновления отображения результатов
void MainWindow::updateResultsDisplay(const QString& results) {
    resultDisplay->setText(results);
}

// Функция для подключения к серверу с обработкой результатов
void MainWindow::connectToServer() {
    QString serverAddress = serverAddressInput->text().trimmed();
    if (!serverAddress.isEmpty()) {
        if (triangleClient) {
            triangleClient->deleteLater();
        }
        triangleClient = new TriangleClient(QUrl(serverAddress), this);

        if (triangleClient) {
            // Подключение сигналов
            connect(triangleClient, &TriangleClient::resultsReceived, this, &MainWindow::displayResults);
            connect(triangleClient, &TriangleClient::logMessage, this, &MainWindow::logMessage);

            serverEnabled = true;
            logMessage("Connecting to server: " + serverAddress);
        } else {
            logMessage("Failed to create TriangleClient.");
        }
    } else {
        logMessage("Server address is empty.");
    }
}

void MainWindow::onConnectedToServer() {
    sendDataAfterAuthorization([this]() {
        QVector<QSharedPointer<triangle>> triangles;
        // Заполните вектор triangles необходимыми данными
        triangleClient->sendTriangleData(triangles);
    });
}

void MainWindow::logMessage(const QString& message) {
    logDisplay->append(message);
}

void MainWindow::updateResultsDisplay(const QJsonObject& results) {
    QJsonDocument doc(results);
    QString resultsString = doc.toJson(QJsonDocument::Indented);
    resultDisplay->setText(resultsString);
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

void MainWindow::openGraphWindow() {
    GraphWindow *graphWindow = new GraphWindow(this);

    QVector<double> x;
    for (int i = 0; i < absEout.size(); ++i) {
        x.append(i);
    }

    qDebug() << "x:" << x;
    qDebug() << "absEout:" << absEout;

    graphWindow->setData(x, absEout);
    graphWindow->setAttribute(Qt::WA_DeleteOnClose);
    graphWindow->show();
}
