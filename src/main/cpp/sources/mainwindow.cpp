#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "parser.h"
#include "raytracer.h"
#include "resultsdialog.h"
#include "graphwindow.h"
#include "logindialog.h"
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QPixmap>
#include <QTextEdit>
#include <QPushButton>
#include <QProgressDialog>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QScrollArea>
#include <QMenuBar>
#include <QMessageBox>
#include <QScopedPointer>

QVector<double> absEout;
QVector<double> normEout;
QCheckBox *anglePortraitCheckBox;
QCheckBox *azimuthPortraitCheckBox;
QCheckBox *rangePortraitCheckBox;
QString storedResults;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , openGLWidget(new OpenGLWidget(this))
    , parser(new Parser(this))
    , rayTracer(new RayTracer())
    , serverEnabled(false)
    , isDarkTheme(true)
{
    ui->setupUi(this);
    setCentralWidget(openGLWidget);

    // Создание меню-бара
    QMenuBar *menuBar = new QMenuBar(this);
    QMenu *fileMenu = new QMenu("Файл", this);

    QAction *openAction = new QAction(QIcon(":/load.png"), "Открыть", this);
    QAction *saveAction = new QAction(QIcon(":/download.png"), "Сохранить", this);
    QAction *exitAction = new QAction("Выход", this);

    fileMenu->addAction(openAction);
    fileMenu->addAction(saveAction);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    menuBar->addMenu(fileMenu);
    setMenuBar(menuBar);

    connect(openAction, &QAction::triggered, this, &MainWindow::loadModel);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveFile);
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);

    // Создание нового меню "Выполнить"
    QMenu *executeMenu = new QMenu("Выполнить", this);
    QAction *performCalculationAction = new QAction(QIcon(":/calculator.png"), "Выполнить расчёт", this);

    executeMenu->addAction(performCalculationAction);
    menuBar->addMenu(executeMenu);

    connect(performCalculationAction, &QAction::triggered, this, &MainWindow::performCalculation);

    // Создание нового меню "Результаты"
    QMenu *resultsMenu = new QMenu("Результаты", this);

    // Пункт меню "Открыть числовые значения"
    QAction *openResultsAction = new QAction("Открыть числовые значения", this);
    resultsMenu->addAction(openResultsAction);

    // Подключаем слот для нового пункта меню
    connect(openResultsAction, &QAction::triggered, this, &MainWindow::openResultsWindow);

    // Пункт меню "Открыть график"
    QAction *openGraphAction = new QAction("Открыть график", this);

    resultsMenu->addAction(openGraphAction);
    menuBar->addMenu(resultsMenu);

    connect(openGraphAction, &QAction::triggered, this, &MainWindow::openGraphWindow);

    // Создание нового меню "Помощь"
    QMenu *helpMenu = new QMenu("Помощь", this);
    QAction *aboutAction = new QAction("О программе", this);

    helpMenu->addAction(aboutAction);
    menuBar->addMenu(helpMenu);

    // connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);

    // Создание нового меню "Настройки"
    QMenu *settingsMenu = new QMenu("Настройки", this);
    QAction *changeThemeAction = new QAction("Сменить тему", this);

    settingsMenu->addAction(changeThemeAction);
    menuBar->addMenu(settingsMenu);

    connect(changeThemeAction, &QAction::triggered, this, &MainWindow::toggleTheme);

    loadTheme(":/darktheme.qss", ":/dark-theme.png", changeThemeAction);

    // Загрузка и применение темной темы
    QFile darkThemeFile(":/darktheme.qss");
    if (darkThemeFile.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(darkThemeFile.readAll());
        qApp->setStyleSheet(styleSheet);
        darkThemeFile.close();
        isDarkTheme = true;  // Устанавливаем начальное состояние
        changeThemeAction->setIcon(QIcon(":/dark-theme.png"));
    } else {
        isDarkTheme = false;
        changeThemeAction->setIcon(QIcon(":/light-theme.png"));
    }

    // Настройка компонентов пользовательского интерфейса
    QDockWidget *dockWidget = new QDockWidget("Панель управления", this);
    addDockWidget(Qt::LeftDockWidgetArea, dockWidget);

    // Создание QScrollArea
    QScrollArea *scrollArea = new QScrollArea(dockWidget);
    scrollArea->setWidgetResizable(true);
    dockWidget->setWidget(scrollArea);

    controlWidget = new QWidget();
    scrollArea->setWidget(controlWidget);
    formLayout = new QFormLayout(controlWidget);

    // Элементы для подключения к серверу
    QGroupBox *serverConnectionGroupBox = new QGroupBox("Подключение к серверу", controlWidget);
    QVBoxLayout *serverConnectionLayout = new QVBoxLayout(serverConnectionGroupBox);

    QHBoxLayout *serverLayout = new QHBoxLayout();
    serverAddressInput = new QLineEdit(serverConnectionGroupBox);
    serverAddressInput->setPlaceholderText("ws://serveraddress:port");
    serverAddressInput->setAlignment(Qt::AlignCenter);
    connectButton = new QPushButton(QIcon(":/connect.png"), "Подключиться", serverConnectionGroupBox);
    QPushButton *disconnectButton = new QPushButton(QIcon(":/disconnect.png"), "Отключиться", serverConnectionGroupBox);

    serverLayout->addWidget(serverAddressInput);
    serverLayout->addWidget(connectButton);
    serverLayout->addWidget(disconnectButton);

    serverConnectionLayout->addLayout(serverLayout);
    serverConnectionGroupBox->setLayout(serverConnectionLayout);

    // Размеры элементов для подключения к серверу
    serverAddressInput->setFixedSize(150, 30);
    connectButton->setFixedSize(110, 30);
    disconnectButton->setFixedSize(110, 30);
    serverConnectionGroupBox->setFixedSize(400, 75);

    formLayout->addRow(serverConnectionGroupBox);

    // Элементы пользовательского интерфейса для ввода параметров
    inputWavelength = new QDoubleSpinBox(controlWidget);
    inputResolution = new QDoubleSpinBox(controlWidget);
    inputPolarization = new QComboBox(controlWidget);

    QGroupBox *portraitTypeGroupBox = new QGroupBox("Портретные типы", controlWidget);
    QVBoxLayout *portraitTypeLayout = new QVBoxLayout(portraitTypeGroupBox);

    azimuthPortraitCheckBox = new QCheckBox("Азимутальный", portraitTypeGroupBox);
    anglePortraitCheckBox = new QCheckBox("Угломестный", portraitTypeGroupBox);
    rangePortraitCheckBox = new QCheckBox("Дальностный", portraitTypeGroupBox);
    inputPolarization->addItems({"Горизонтальный", "Вертикальный", "Круговой"});

    portraitTypeLayout->addWidget(anglePortraitCheckBox);
    portraitTypeLayout->addWidget(azimuthPortraitCheckBox);
    portraitTypeLayout->addWidget(rangePortraitCheckBox);

    portraitTypeGroupBox->setFixedSize(125, 150);
    azimuthPortraitCheckBox->setFixedSize(180, 30);
    anglePortraitCheckBox->setFixedSize(180, 30);
    rangePortraitCheckBox->setFixedSize(180, 30);

    portraitTypeGroupBox->setLayout(portraitTypeLayout);
    formLayout->addRow(portraitTypeGroupBox);

    formLayout->addRow(new QLabel("Длина волны (nm):"), inputWavelength);
    formLayout->addRow(new QLabel("Разрешение (m):"), inputResolution);
    formLayout->addRow(new QLabel("Поляризация:"), inputPolarization);

    logDisplay = new QTextEdit(controlWidget);
    logDisplay->setReadOnly(true);
    formLayout->addRow(new QLabel("Log:"), logDisplay);

    // Элементы пользовательского интерфейса для отображения результатов
    buttonSaveResults = new QPushButton("Save Results", controlWidget);
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

    // Создание нового виджета для двумерного портрета
    portraitWidget = new PortraitWidget(this);
    QDockWidget *portraitDockWidget = new QDockWidget("2D Portrait", this);
    portraitDockWidget->setWidget(portraitWidget);
    addDockWidget(Qt::RightDockWidgetArea, portraitDockWidget);

    // Кнопка для отображения двумерного портрета
    QPushButton *buttonShowPortrait = new QPushButton("Показать 2D портрет", controlWidget);
    formLayout->addRow(buttonShowPortrait);
    connect(buttonShowPortrait, &QPushButton::clicked, this, &MainWindow::showPortrait);

    // this->resize(1920, 1080);
    this->resize(1280, 720);

    // Соединения
    connect(buttonApplyRotation, &QPushButton::clicked, this, &MainWindow::applyRotation);
    connect(buttonResetRotation, &QPushButton::clicked, this, &MainWindow::resetRotation);
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

    inputResolution->setFixedSize(100, 30);
    inputWavelength->setFixedSize(100, 30);
    inputPolarization->setFixedSize(100, 30);

    // Элементы пользовательского интерфейса для отображения результатов
    logDisplay->setFixedSize(200, 50);
    buttonSaveResults->setFixedSize(150, 30);

    // Элементы для ввода углов поворота
    inputRotationX->setFixedSize(80, 30);
    inputRotationY->setFixedSize(80, 30);
    inputRotationZ->setFixedSize(80, 30);
    buttonApplyRotation->setFixedSize(150, 30);
    buttonResetRotation->setFixedSize(150, 30);

    // Кнопка для отображения двумерного портрета
    buttonShowPortrait->setFixedSize(150, 30);

}

MainWindow::~MainWindow() {
    delete ui;
}

// Функция для загрузки модели
void MainWindow::loadModel() {
    QString fileName = QFileDialog::getOpenFileName(this, "Open 3D model", "", "OBJ Files (*.obj)");
    if (!fileName.isEmpty()) {
        // lineEditFilePath->setText(fileName);
        openGLWidget->clearScene();
        parser->clearData();
        parser->readFromObjFile(fileName);

        // Проверка наличия треугольников после загрузки файла
        if (openGLWidget->getTriangles().isEmpty()) {
            logMessage("Ошибка: файл не содержит корректных данных объекта. Пожалуйста, загрузите корректный файл.");
            // lineEditFilePath->clear();
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
        QJsonArray innerArray = array.at(i).toArray();

        switch (depth) {
        case 1:
            if (!innerArray.isEmpty()) {
                container.append(innerArray.at(0).toDouble());
            } else {
                qDebug() << "Inner array is empty or not found";
            }
            break;

        case 2:
            for (const QJsonValue &val : innerArray) {
                container.append(val.toDouble());
            }
            break;

        case 3:
            for (const QJsonValue &innerValue : innerArray) {
                QJsonArray innermostArray = innerValue.toArray();
                for (const QJsonValue &val : innermostArray) {
                    container.append(val.toDouble());
                }
            }
            break;

        case 4:
            for (const QJsonValue &innerValue : innerArray) {
                QJsonArray innermostArray = innerValue.toArray();
                for (const QJsonValue &innermostInnerValue : innermostArray) {
                    if (innermostInnerValue.isArray() && !innermostInnerValue.toArray().isEmpty()) {
                        container.append(innermostInnerValue.toArray().at(0).toDouble());
                    }
                }
            }
            break;

        default:
            qDebug() << "Unsupported array depth";
            break;
        }
    }
}

// Функция для отображения результатов
void MainWindow::displayResults(const QJsonObject &results) {
    // Преобразование JSON-объекта в JSON-документ для дальнейшего использования
    QJsonDocument doc(results);
    storedResults = doc.toJson(QJsonDocument::Indented); // Сохранение результатов
    logMessage("Числовые значения с сервера получены и сохранены.");

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

    // Подготовка данных для двумерного портрета
    QVector<double> xData, yData, zData;
    int totalSteps = static_cast<int>(sqrt(absEout.size())); // Предполагаем, что данные квадратные
    for (int i = 0; i < absEout.size(); ++i) {
        int angleIndex = i % totalSteps; // Индекс угла
        int azimuthIndex = i / totalSteps; // Индекс азимута
        double angle = calculateAngle(angleIndex, totalSteps);
        double azimuth = calculateAzimuth(azimuthIndex, totalSteps);
        double eoutValue = absEout[i];
        xData.append(angle);
        yData.append(azimuth);
        zData.append(eoutValue);
    }

    portraitWidget->setData(xData, yData, zData);
    qDebug() << "2D Portrait data prepared";
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
    // Создаем диалоговое окно для ввода логина и пароля
    LoginDialog dialog(this);

    // Показываем диалоговое окно и ждем, пока пользователь не введет данные и не нажмет "Login"
    if (dialog.exec() == QDialog::Accepted) {
        // Получаем введенные пользователем логин и пароль
        QString username = dialog.getUsername();
        QString password = dialog.getPassword();

        // Вызываем метод authorize у triangleClient с введенными данными
        triangleClient->authorize(username, password);
    }
}

void MainWindow::sendDataAfterAuthorization(std::function<void()> sendDataFunc) {
    if (!triangleClient || !triangleClient->isConnected()) {
        logDisplay->setText("Для начала подключитесь к серверу.");
        return;
    }

    if (!triangleClient->isAuthorized()) {
        authorizeClient();  // Вызываем метод авторизации, если не авторизованы
        QTimer::singleShot(2000, this, [this, sendDataFunc]() {
            if (triangleClient->isConnected() && triangleClient->isAuthorized()) {
                sendDataFunc();  // Отправляем данные после успешной авторизации
            } else {
                logDisplay->setText("Ошибка авторизации.");
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

// Функция для сохранения результатов
void MainWindow::saveResults() {
    QString fileName = QFileDialog::getSaveFileName(this, "Save Results", "", "Text Files (*.txt)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << logDisplay->toPlainText();
        }
    }
}

void MainWindow::openResultsWindow() {
    if (storedResults.isEmpty()) {
        logMessage("Нет доступных числовых значений.");
        return;
    }

    QScopedPointer<ResultsDialog> resultsDialog(new ResultsDialog(this));
    resultsDialog->setResults(storedResults);
    resultsDialog->exec();
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

double MainWindow::calculateAngle(int index, int totalSteps) {
    double angleRange = 180.0; // Диапазон углов в градусах
    double stepSize = angleRange / (totalSteps - 1); // Шаг угла
    return index * stepSize;
}

double MainWindow::calculateAzimuth(int index, int totalSteps) {
    double azimuthRange = 360.0; // Диапазон азимутов в градусах
    double stepSize = azimuthRange / (totalSteps - 1); // Шаг азимута
    return index * stepSize;
}

void MainWindow::showPortrait() {
    QVector<double> xData, yData, zData;
    int totalSteps = static_cast<int>(sqrt(absEout.size())); // Предполагаем, что данные квадратные
    for (int i = 0; i < absEout.size(); ++i) {
        int angleIndex = i % totalSteps;
        int azimuthIndex = i / totalSteps;
        double angle = calculateAngle(angleIndex, totalSteps);
        double azimuth = calculateAzimuth(azimuthIndex, totalSteps);
        double eoutValue = absEout[i];
        xData.append(angle);
        yData.append(azimuth);
        zData.append(eoutValue);
    }

    portraitWidget->setData(xData, yData, zData);
    qDebug() << "2D Portrait data set";
}

// Метод для переключения темы
void MainWindow::toggleTheme() {
    if (isDarkTheme) {
        loadTheme(":/lighttheme.qss", ":/light-theme.png", qobject_cast<QAction *>(sender()));
    } else {
        loadTheme(":/darktheme.qss", ":/dark-theme.png", qobject_cast<QAction *>(sender()));
    }
    isDarkTheme = !isDarkTheme;  // Переключение состояния темы
}

void MainWindow::loadTheme(const QString &themePath, const QString &iconPath, QAction *action) {
    QFile file(themePath);
    if (file.open(QFile::ReadOnly)) {
        QString styleSheet = QLatin1String(file.readAll());
        qApp->setStyleSheet(styleSheet);
        file.close();
    } else {
        qApp->setStyleSheet("");
    }
    if (action) {
        action->setIcon(QIcon(iconPath));
    }
}

void MainWindow::saveFile() {
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить файл", "", "Все файлы (*.*);;Текстовые файлы (*.txt)");
    if (!fileName.isEmpty()) {
        // Ваш код для обработки сохранения файла
        logMessage("Файл сохранен: " + fileName);
    }
}
