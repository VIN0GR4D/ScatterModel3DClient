#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "parser.h"
#include "raytracer.h"
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
QVector<double> normEout;
QCheckBox *anglePortraitCheckBox;
QCheckBox *azimuthPortraitCheckBox;
QCheckBox *rangePortraitCheckBox;

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
    buttonLoadModel = new QPushButton(QIcon(":/load.png"), "Загрузить объект", controlWidget);
    formLayout->addRow(new QLabel("Файл объекта:"), lineEditFilePath);
    formLayout->addRow(buttonLoadModel);

    // Элементы пользовательского интерфейса для ввода параметров
    inputWavelength = new QDoubleSpinBox(controlWidget);
    inputResolution = new QDoubleSpinBox(controlWidget);
    inputPolarization = new QComboBox(controlWidget);
    buttonPerformCalculation = new QPushButton(QIcon(":/calculator.png"),"Выполнить расчет", controlWidget);

    QGroupBox *portraitTypeGroupBox = new QGroupBox("Портретные типы", controlWidget);
    QVBoxLayout *portraitTypeLayout = new QVBoxLayout(portraitTypeGroupBox);

    azimuthPortraitCheckBox = new QCheckBox("Азимутальный", portraitTypeGroupBox);
    anglePortraitCheckBox = new QCheckBox("Угломестный", portraitTypeGroupBox);
    rangePortraitCheckBox = new QCheckBox("Дальностный", portraitTypeGroupBox);
    inputPolarization->addItems({"Горизонтальный", "Вертикальный", "Круговой"});

    portraitTypeLayout->addWidget(anglePortraitCheckBox);
    portraitTypeLayout->addWidget(azimuthPortraitCheckBox);
    portraitTypeLayout->addWidget(rangePortraitCheckBox);

    portraitTypeGroupBox->setLayout(portraitTypeLayout);
    formLayout->addRow(portraitTypeGroupBox);

    formLayout->addRow(new QLabel("Длина волны (nm):"), inputWavelength);
    formLayout->addRow(new QLabel("Разрешение (m):"), inputResolution);
    formLayout->addRow(new QLabel("Поляризация:"), inputPolarization);
    formLayout->addRow(buttonPerformCalculation);

    // Элементы для подключения к серверу
    serverAddressInput = new QLineEdit(controlWidget);
    serverAddressInput->setPlaceholderText("ws://yourserveraddress:port");
    connectButton = new QPushButton(QIcon(":/connect.png"),"Connect", controlWidget);
    QPushButton *disconnectButton = new QPushButton(QIcon(":/disconnect.png"),"Disconnect", controlWidget);
    formLayout->addRow(new QLabel("Server Address:"), serverAddressInput);
    formLayout->addRow(connectButton);
    formLayout->addRow(disconnectButton);

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
    connect(disconnectButton, &QPushButton::clicked, this, &MainWindow::disconnectFromServer);
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

        // Проверка наличия треугольников после загрузки файла
        if (openGLWidget->getTriangles().isEmpty()) {
            logMessage("Ошибка: файл не содержит корректных данных объекта. Пожалуйста, загрузите корректный файл.");
            lineEditFilePath->clear();
        } else {
            logMessage("Файл успешно загружен.");
        }
    } else {
        logMessage("Ошибка: файл не был выбран.");
        return;
    }
}

// Вспомогательная функция для извлечения значений из вложенных массивов
void MainWindow::extractValues(const QJsonArray &array, QVector<double> &container, int depth) {
    for (int i = 0; i < array.size(); ++i) {
        const QJsonValue &value = array.at(i);
        QJsonArray innerArray = value.toArray();
        if (depth == 1) {
            if (!innerArray.isEmpty()) {
                double val = innerArray.at(0).toDouble();
                container.append(val);
            } else {
                qDebug() << "Inner array is empty or not found";
            }
        } else if (depth == 2) {
            for (int j = 0; j < innerArray.size(); ++j) {
                double val = innerArray.at(j).toDouble();
                container.append(val);
            }
        } else if (depth == 3) {
            for (int j = 0; j < innerArray.size(); ++j) {
                const QJsonValue &innerValue = innerArray.at(j);
                QJsonArray innermostArray = innerValue.toArray();
                for (int k = 0; k < innermostArray.size(); ++k) {
                    double val = innermostArray.at(k).toDouble();
                    container.append(val);
                }
            }
        } else if (depth == 4) {
            for (int j = 0; j < innerArray.size(); ++j) {
                const QJsonValue &innerValue = innerArray.at(j);
                QJsonArray innermostArray = innerValue.toArray();
                for (int k = 0; k < innermostArray.size(); ++k) {
                    const QJsonValue &innermostInnerValue = innermostArray.at(k);
                    if (innermostInnerValue.isArray() && !innermostInnerValue.toArray().isEmpty()) {
                        double val = innermostInnerValue.toArray().at(0).toDouble();
                        container.append(val);
                    }
                }
            }
        } else {
            qDebug() << "Unsupported array depth";
        }
    }
}

// Функция для отображения результатов
void MainWindow::displayResults(const QJsonObject &results) {
    // Преобразование JSON-объекта в JSON-документ для дальнейшего использования
    QJsonDocument doc(results);
    QString resultsString = doc.toJson(QJsonDocument::Indented);
    resultDisplay->setText(resultsString);

    // Извлечение массивов absEout и normEout из JSON-объекта результатов
    QJsonArray absEoutArray = results["absEout"].toArray();
    QJsonArray normEoutArray = results["normEout"].toArray();

    // Очистка текущего содержимого векторов absEout и normEout
    absEout.clear();
    normEout.clear();

    // Проверка, какие чекбоксы отмечены и установка соответствующей глубины вложенности
    if (anglePortraitCheckBox->isChecked()) {
        // Для угломестного типа также используется двойная вложенность
        extractValues(absEoutArray, absEout, 3);
        extractValues(normEoutArray, normEout, 3);
    }
    if (azimuthPortraitCheckBox->isChecked()) {
        // Для азимутального типа используется двойная вложенность
        extractValues(absEoutArray, absEout, 3);
        extractValues(normEoutArray, normEout, 3);
    }
    if (rangePortraitCheckBox->isChecked()) {
        // Для дальностного типа требуется тройная вложенность
        extractValues(absEoutArray, absEout, 3);
        extractValues(normEoutArray, normEout, 3);
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

// Метод для авторизации
void MainWindow::authorizeClient() {
    QString username = "user";  // Логин
    QString password = "user";  // Пароль
    triangleClient->authorize(username, password);
}

void MainWindow::sendDataAfterAuthorization(std::function<void()> sendDataFunc) {
    if (!triangleClient || !triangleClient->isConnected()) {
        resultDisplay->setText("Для начала подключитесь к серверу.");
        return;
    }

    if (!triangleClient->isAuthorized()) {
        authorizeClient();  // Вызываем метод авторизации, если не авторизованы
        QTimer::singleShot(2000, this, [this, sendDataFunc]() {
            if (triangleClient->isConnected() && triangleClient->isAuthorized()) {
                sendDataFunc();  // Отправляем данные после успешной авторизации
            } else {
                resultDisplay->setText("Ошибка авторизации.");
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
    if (lineEditFilePath->text().isEmpty()) {
        logMessage("Ошибка: объект не загружен. Пожалуйста, загрузите объект перед выполнением расчета.");
        return;
    }

    QVector<QSharedPointer<triangle>> triangles = openGLWidget->getTriangles();
    if (triangles.isEmpty()) {
        logMessage("Ошибка: загруженный файл не содержит корректных данных объекта.");
        return;
    }

    if (!anglePortraitCheckBox->isChecked() && !azimuthPortraitCheckBox->isChecked() && !rangePortraitCheckBox->isChecked()) {
        logMessage("Ошибка: тип радиопортрета не задан. Пожалуйста, выберите хотя бы один тип радиопортрета.");
        return;
    }

    double wavelength = inputWavelength->value();
    double resolution = inputResolution->value();
    QString polarizationText = inputPolarization->currentText();

    int polarRadiation = 0;
    int polarRecive = 0;
    if (polarizationText == "Горизонтальный") {
        polarRadiation = 1;
        polarRecive = 1;
    } else if (polarizationText == "Круговой") {
        polarRadiation = 2;
        polarRecive = 2;
    }

    bool typeAngle = anglePortraitCheckBox->isChecked();
    bool typeAzimut = azimuthPortraitCheckBox->isChecked();
    bool typeLength = rangePortraitCheckBox->isChecked();

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
    modelData["visbleTriangles"] = visibleTrianglesArray;
    modelData["freqBand"] = 5;
    modelData["polarRadiation"] = polarRadiation;
    modelData["polarRecive"] = polarRecive;
    modelData["typeAngle"] = typeAngle;
    modelData["typeAzimut"] = typeAzimut;
    modelData["typeLength"] = typeLength;
    modelData["pplane"] = false;
    modelData["directVector"] = vectorToJson(QSharedPointer<rVect>::create(directVector));
    modelData["resolution"] = resolution;
    modelData["wavelength"] = wavelength;

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

void MainWindow::disconnectFromServer() {
    if (triangleClient && triangleClient->isConnected()) {
        triangleClient->disconnectFromServer();
        logMessage("Disconnected from server.");
        serverEnabled = false;
    } else {
        logMessage("Not connected to any server.");
    }
}

void MainWindow::logMessage(const QString& message) {
    logDisplay->append(QTime::currentTime().toString("HH:mm:ss") + " - " + message);
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
    qDebug() << "normEout:" << normEout;

    graphWindow->setData(x, absEout, normEout);
    graphWindow->setAttribute(Qt::WA_DeleteOnClose);
    graphWindow->show();
}
