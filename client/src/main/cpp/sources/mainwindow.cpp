#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "parser.h"
#include "raytracer.h"
#include "resultsdialog.h"
#include "aboutdialog.h"
#include "graphwindow.h"
#include "logindialog.h"
#include "portraitwindow.h"
#include "graph3dwindow.h"
#include "scatterplot3dwindow.h"
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QLabel>
#include <QDoubleSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QtConcurrent/QtConcurrent>
#include <QFutureWatcher>
#include <QScrollArea>
#include <QMenuBar>
#include <QMessageBox>
#include <QScopedPointer>
#include <memory>
#include <QSlider>

QVector<double> absEout;
QVector<double> normEout;
QCheckBox *anglePortraitCheckBox;
QCheckBox *azimuthPortraitCheckBox;
QCheckBox *rangePortraitCheckBox;
QString storedResults;
QComboBox *freqBandComboBox;
QCheckBox *pplaneCheckBox;

QComboBox *radiationPolarizationComboBox;
QComboBox *receivePolarizationComboBox;

QVector<QVector<double>> absEout2D;
QVector<QVector<double>> normEout2D;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , openGLWidget(new OpenGLWidget(this))
    , parser(new Parser(this))
    , rayTracer(std::make_unique<RayTracer>())
    , serverEnabled(false)
    , graph3DWindow(new Graph3DWindow(this))
    , portraitWindow(new PortraitWindow(this))
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
    openResultsAction->setEnabled(true);
    resultsMenu->addAction(openResultsAction);
    connect(openResultsAction, &QAction::triggered, this, &MainWindow::openResultsWindow);

    // Пункт меню "Открыть график"
    QAction *openGraphAction = new QAction("Открыть график", this);
    openGraphAction->setEnabled(true);
    resultsMenu->addAction(openGraphAction);
    connect(openGraphAction, &QAction::triggered, this, &MainWindow::openGraphWindow);

    // Пункт меню "Показать 2D портрет"
    QAction *showPortraitAction = new QAction("Показать 2D портрет", this);
    showPortraitAction->setEnabled(true);
    resultsMenu->addAction(showPortraitAction);
    connect(showPortraitAction, &QAction::triggered, this, &MainWindow::showPortrait);

    QAction *showGraph3DAction = new QAction("Показать 3D Модель", this);
    showGraph3DAction->setEnabled(true);
    resultsMenu->addAction(showGraph3DAction);
    connect(showGraph3DAction, &QAction::triggered, this, &MainWindow::showGraph3D);

    // Пункт меню "Показать Scatter Plot 3D"
    QAction *openScatterPlot3DAction = new QAction("Показать Scatter Plot 3D", this);
    openScatterPlot3DAction->setEnabled(true);
    resultsMenu->addAction(openScatterPlot3DAction);
    connect(openScatterPlot3DAction, &QAction::triggered, this, &MainWindow::openScatterPlot3DWindow);

    menuBar->addMenu(resultsMenu);

    // Создание нового меню "Помощь"
    QMenu *helpMenu = new QMenu("Помощь", this);
    QAction *aboutAction = new QAction("О программе", this);

    helpMenu->addAction(aboutAction);
    menuBar->addMenu(helpMenu);

    connect(aboutAction, &QAction::triggered, this, &MainWindow::showAboutDialog);

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
    QGroupBox *radiationPolarizationGroupBox = new QGroupBox("Излучения", controlWidget);
    QVBoxLayout *radiationPolarizationLayout = new QVBoxLayout(radiationPolarizationGroupBox);
    radiationPolarizationComboBox = new QComboBox();
    radiationPolarizationComboBox->addItems({"Горизонтальный", "Вертикальный", "Круговой"});
    radiationPolarizationLayout->addWidget(radiationPolarizationComboBox);
    radiationPolarizationGroupBox->setLayout(radiationPolarizationLayout);

    QGroupBox *receivePolarizationGroupBox = new QGroupBox("Приёма", controlWidget);
    QVBoxLayout *receivePolarizationLayout = new QVBoxLayout(receivePolarizationGroupBox);
    receivePolarizationComboBox = new QComboBox();
    receivePolarizationComboBox->addItems({"Горизонтальный", "Вертикальный", "Круговой"});
    receivePolarizationLayout->addWidget(receivePolarizationComboBox);
    receivePolarizationGroupBox->setLayout(receivePolarizationLayout);

    // Компоновка поляризации излучения и приёма на одном уровне
    QHBoxLayout *polarizationLayout = new QHBoxLayout();
    polarizationLayout->addWidget(radiationPolarizationGroupBox);
    polarizationLayout->addWidget(receivePolarizationGroupBox);

    QGroupBox *polarizationGroupBox = new QGroupBox("Поляризация", controlWidget);
    polarizationGroupBox->setLayout(polarizationLayout);

    // Получаем рекомендованный размер по оси X
    int recommendedWidth = polarizationGroupBox->sizeHint().width();

    // Устанавливаем фиксированный размер по оси Y и оставляем рекомендованный размер по оси X
    polarizationGroupBox->setFixedSize(recommendedWidth, 150);

    // Группа для диапазона частот и подстилающей поверхности
    freqBandComboBox = new QComboBox(controlWidget);
    freqBandComboBox->addItems({"P-диапазон (400-450 МГц)", "L-диапазон (1-1.5 ГГц)", "S-диапазон (2.75-3.15 ГГц)", "C-диапазон (5-5.5 ГГц)", "X-диапазон (9-10 ГГц)", "Ka-диапазон (36.5-38.5 ГГц)"});

    pplaneCheckBox = new QCheckBox("Включить подстилающую поверхность", controlWidget);

    QGroupBox *frequencyAndPlaneGroupBox = new QGroupBox("Параметры", controlWidget);
    QFormLayout *frequencyAndPlaneLayout = new QFormLayout(frequencyAndPlaneGroupBox);
    frequencyAndPlaneLayout->addRow(new QLabel("Диапазон частот:"), freqBandComboBox);
    frequencyAndPlaneLayout->addRow(pplaneCheckBox);
    frequencyAndPlaneGroupBox->setLayout(frequencyAndPlaneLayout);

    // Настройка размеров элементов для диапазона частот и подстилающей поверхности
    freqBandComboBox->setFixedSize(250, 30);
    pplaneCheckBox->setFixedSize(235, 30);
    frequencyAndPlaneGroupBox->setFixedSize(400, 100);

    // Группа для портретных типов
    QGroupBox *portraitTypeGroupBox = new QGroupBox("Портретные типы", controlWidget);
    QVBoxLayout *portraitTypeLayout = new QVBoxLayout(portraitTypeGroupBox);

    azimuthPortraitCheckBox = new QCheckBox("Азимутальный", portraitTypeGroupBox);
    anglePortraitCheckBox = new QCheckBox("Угломестный", portraitTypeGroupBox);
    rangePortraitCheckBox = new QCheckBox("Дальностный", portraitTypeGroupBox);

    portraitTypeLayout->addWidget(anglePortraitCheckBox);
    portraitTypeLayout->addWidget(azimuthPortraitCheckBox);
    portraitTypeLayout->addWidget(rangePortraitCheckBox);

    azimuthPortraitCheckBox->setFixedSize(180, 30);
    anglePortraitCheckBox->setFixedSize(180, 30);
    rangePortraitCheckBox->setFixedSize(180, 30);
    portraitTypeGroupBox->setFixedSize(125, 150);

    portraitTypeGroupBox->setLayout(portraitTypeLayout);

    // Компоновка параметров и портретных типов в одну строку
    QHBoxLayout *parametersAndPortraitLayout = new QHBoxLayout();
    parametersAndPortraitLayout->addWidget(portraitTypeGroupBox);
    parametersAndPortraitLayout->addWidget(polarizationGroupBox);

    QWidget *parametersAndPortraitWidget = new QWidget();
    parametersAndPortraitWidget->setLayout(parametersAndPortraitLayout);

    // Устанавливаем фиксированный размер для parametersAndPortraitWidget
    parametersAndPortraitWidget->setFixedSize(parametersAndPortraitLayout->sizeHint());
    formLayout->addRow(parametersAndPortraitWidget);

    // Добавляем группу для диапазона частот и подстилающей поверхности в основной макет
    formLayout->addRow(frequencyAndPlaneGroupBox);

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

    QGroupBox *rotationGroupBox = new QGroupBox("Поворот", controlWidget);
    QHBoxLayout *rotationMainLayout = new QHBoxLayout(rotationGroupBox);

    // Создаем QGridLayout для меток и ввода поворота
    QGridLayout *rotationGridLayout = new QGridLayout();
    rotationGridLayout->addWidget(new QLabel("Поворот по X:"), 0, 0);
    rotationGridLayout->addWidget(inputRotationX, 0, 1);
    rotationGridLayout->addWidget(new QLabel("Поворот по Y:"), 1, 0);
    rotationGridLayout->addWidget(inputRotationY, 1, 1);
    rotationGridLayout->addWidget(new QLabel("Поворот по Z:"), 2, 0);
    rotationGridLayout->addWidget(inputRotationZ, 2, 1);

    // Создаем QVBoxLayout для кнопок
    QVBoxLayout *buttonLayout = new QVBoxLayout();
    buttonLayout->addStretch(); // Добавляем растягивающий элемент сверху
    buttonLayout->addWidget(buttonApplyRotation);
    buttonLayout->addWidget(buttonResetRotation);
    buttonLayout->addStretch(); // Добавляем растягивающий элемент снизу

    // Добавляем оба layout в rotationMainLayout
    rotationMainLayout->addLayout(rotationGridLayout);
    rotationMainLayout->addSpacing(10); // Добавляем расстояние между столбцами
    rotationMainLayout->addLayout(buttonLayout);

    // Устанавливаем layout для группы
    rotationGroupBox->setLayout(rotationMainLayout);
    formLayout->addRow(rotationGroupBox);

    rotationGroupBox->setFixedSize(400, 125);

    connect(openGLWidget, &OpenGLWidget::rotationChanged, this, [this](float x, float y, float z) {
        updateRotationX(x);
        updateRotationY(y);
        updateRotationZ(z);
    });

    // Журнал действий
    QGroupBox *logGroupBox = new QGroupBox("Журнал действий", controlWidget);
    QVBoxLayout *logLayout = new QVBoxLayout(logGroupBox);
    logDisplay = new QTextEdit(logGroupBox);
    logDisplay->setReadOnly(true);
    logLayout->addWidget(logDisplay);

    // Кнопка для сохранения журнала действий
    QPushButton *saveLogButton = new QPushButton("Сохранить журнал действий", logGroupBox);
    logLayout->addWidget(saveLogButton);
    connect(saveLogButton, &QPushButton::clicked, this, &MainWindow::saveLog);

    logGroupBox->setLayout(logLayout);
    formLayout->addRow(logGroupBox);
    logGroupBox->setFixedSize(400, 230);
    logDisplay->setFixedSize(380, 150);
    saveLogButton->setFixedSize(380, 30);

    setWindowTitle("ScatterModel3DClient");

    this->resize(1280, 720);

    // Соединения
    connect(buttonApplyRotation, &QPushButton::clicked, this, &MainWindow::applyRotation);
    connect(buttonResetRotation, &QPushButton::clicked, this, &MainWindow::resetRotation);
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::connectToServer);
    connect(disconnectButton, &QPushButton::clicked, this, &MainWindow::disconnectFromServer);
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
    // Открываем диалоговое окно для выбора файла с 3D моделью формата OBJ
    QString fileName = QFileDialog::getOpenFileName(this, "Открыть 3D модель", "", "OBJ Files (*.obj)");
    if (!fileName.isEmpty()) {
        // Выводим сообщение в лог о начале загрузки файла
        logMessage("Начата загрузка файла: " + fileName);

        // Создаем объект QFutureWatcher для отслеживания завершения асинхронной задачи
        QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);
        // Соединяем сигнал finished объекта QFutureWatcher с лямбда-функцией для обработки завершения задачи
        connect(watcher, &QFutureWatcher<void>::finished, this, [=]() {
            // Освобождаем память, удаляя объект QFutureWatcher
            watcher->deleteLater();
        });

        // Запускаем асинхронную задачу с использованием QtConcurrent::run
        QFuture<void> future = QtConcurrent::run([=]() {
            QFile file(fileName);
            // Пытаемся открыть файл для чтения, если не удалось - выводим сообщение об ошибке в лог
            if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QMetaObject::invokeMethod(this, [=]() {
                        logMessage("Ошибка: не удалось открыть файл.");
                    }, Qt::QueuedConnection);
                return;
            }

            // Читаем содержимое файла
            QTextStream in(&file);
            // Очищаем текущую сцену и данные парсера
            openGLWidget->clearScene();
            parser->clearData();
            // Парсим содержимое файла и загружаем данные в OpenGL виджет
            parser->readFromObjFile(fileName);

            // Проверка наличия треугольников
            QMetaObject::invokeMethod(this, [=]() {
                    if (openGLWidget->getTriangles().isEmpty()) {
                        logMessage("Ошибка: файл не содержит корректных данных объекта. Пожалуйста, загрузите корректный файл.");
                    } else {
                        logMessage("Файл успешно загружен.");
                    }
                }, Qt::QueuedConnection);
        });

        // Связываем QFutureWatcher с асинхронной задачей
        watcher->setFuture(future);
    } else {
        // Если файл не был выбран, выводим сообщение об ошибке в лог
        logMessage("Ошибка: файл не был выбран.");
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

void MainWindow::extract2DValues(const QJsonArray &array, QVector<QVector<double>> &container) {
    container.clear();  // Очистим контейнер перед заполнением
    for (int i = 0; i < array.size(); ++i) {
        QJsonArray innerArray = array.at(i).toArray();
        QVector<double> row;
        for (const QJsonValue &val : innerArray) {
            if (val.isArray()) {
                QJsonArray innermostArray = val.toArray();
                for (const QJsonValue &innerVal : innermostArray) {
                    row.append(innerVal.toDouble());
                }
            } else {
                row.append(val.toDouble());
            }
        }
        container.append(row);
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
    absEout2D.clear();
    normEout2D.clear();

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

    // Извлечение двумерных массивов
    extract2DValues(absEoutArray, absEout2D);
    extract2DValues(normEoutArray, normEout2D);

    // Debug вывод данных
    qDebug() << "absEout2D:" << absEout2D;
    qDebug() << "normEout2D:" << normEout2D;
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

rVect MainWindow::calculateDirectVectorFromRotation() {
    // Получаем углы поворота от пользователя
    float rotationX = inputRotationX->value();
    float rotationY = inputRotationY->value();
    float rotationZ = inputRotationZ->value();

    // Преобразуем углы в радианы
    float radX = qDegreesToRadians(rotationX);
    float radY = qDegreesToRadians(rotationY);
    float radZ = qDegreesToRadians(rotationZ);

    // Вычисляем компоненты вектора направления на основе углов Эйлера
    float x = cos(radY) * cos(radZ);
    float y = sin(radX) * sin(radY) * cos(radZ) - cos(radX) * sin(radZ);
    float z = cos(radX) * sin(radY) * cos(radZ) + sin(radX) * sin(radZ);

    // Нормализуем вектор, чтобы его длина была равна 1
    rVect directVector(x, y, z);
    directVector.normalize();

    return directVector;
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
// void MainWindow::performCalculation() {
//     QVector<QSharedPointer<triangle>> triangles = openGLWidget->getTriangles();
//     if (triangles.isEmpty()) {
//         logMessage("Ошибка: загруженный файл не содержит корректных данных объекта.");
//         return;
//     }

//     if (!anglePortraitCheckBox->isChecked() && !azimuthPortraitCheckBox->isChecked() && !rangePortraitCheckBox->isChecked()) {
//         logMessage("Ошибка: тип радиопортрета не задан. Пожалуйста, выберите хотя бы один тип радиопортрета.");
//         return;
//     }

//     int polarRadiation = radiationPolarizationComboBox->currentIndex();
//     int polarRecive = receivePolarizationComboBox->currentIndex();

//     bool typeAngle = anglePortraitCheckBox->isChecked();
//     bool typeAzimut = azimuthPortraitCheckBox->isChecked();
//     bool typeLength = rangePortraitCheckBox->isChecked();

//     // Получение значения диапазона частот
//     int freqBand = freqBandComboBox->currentIndex();

//     // Получение значения подстилающей поверхности
//     bool pplane = pplaneCheckBox->isChecked();

//     QJsonObject dataObject;
//     QJsonArray visibleTrianglesArray;

//     int index = 0;
//     for (const auto& tri : triangles) {
//         QSharedPointer<rVect> v1 = tri->getV1();
//         QSharedPointer<rVect> v2 = tri->getV2();
//         QSharedPointer<rVect> v3 = tri->getV3();

//         dataObject[QString::number(index++)] = v1->getX();
//         dataObject[QString::number(index++)] = v1->getY();
//         dataObject[QString::number(index++)] = v1->getZ();
//         dataObject[QString::number(index++)] = v2->getX();
//         dataObject[QString::number(index++)] = v2->getY();
//         dataObject[QString::number(index++)] = v2->getZ();
//         dataObject[QString::number(index++)] = v3->getX();
//         dataObject[QString::number(index++)] = v3->getY();
//         dataObject[QString::number(index++)] = v3->getZ();

//         visibleTrianglesArray.append(tri->getVisible());

//         qDebug() << "Triangle" << index / 9 << ":"
//                  << "V1(" << v1->getX() << "," << v1->getY() << "," << v1->getZ() << "),"
//                  << "V2(" << v2->getX() << "," << v2->getY() << "," << v2->getZ() << "),"
//                  << "V3(" << v3->getX() << "," << v3->getY() << "," << v3->getZ() << "),"
//                  << "Visible:" << tri->getVisible();
//     }

//     QVector3D cameraPosition = openGLWidget->getCameraPosition();
//     rVect directVector = openGLWidget->QVector3DToRVect(cameraPosition);

//     QJsonObject modelData;
//     modelData["data"] = dataObject;
//     modelData["visibleTriangles"] = visibleTrianglesArray;
//     modelData["freqBand"] = freqBand;
//     modelData["polarRadiation"] = polarRadiation;
//     modelData["polarRecive"] = polarRecive;
//     modelData["typeAngle"] = typeAngle;
//     modelData["typeAzimut"] = typeAzimut;
//     modelData["typeLength"] = typeLength;
//     modelData["pplane"] = pplane;
//     modelData["directVector"] = vectorToJson(QSharedPointer<rVect>::create(directVector));

//     // QJsonDocument doc(modelData);
//     // qDebug() << "Model data to be sent to server:" << doc.toJson(QJsonDocument::Indented);

//     sendDataAfterAuthorization([this, modelData]() {
//         triangleClient->sendModelData(modelData);
//     });
// }

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

    int polarRadiation = radiationPolarizationComboBox->currentIndex();
    int polarRecive = receivePolarizationComboBox->currentIndex();

    bool typeAngle = anglePortraitCheckBox->isChecked();
    bool typeAzimut = azimuthPortraitCheckBox->isChecked();
    bool typeLength = rangePortraitCheckBox->isChecked();

    // Получение значения диапазона частот
    freqBand = freqBandComboBox->currentIndex();

    // Получение значения подстилающей поверхности
    bool pplane = pplaneCheckBox->isChecked();

    QJsonObject dataObject;
    QJsonArray coordinateArray;
    QJsonArray visibleTrianglesArray;

    for (const auto& tri : triangles) {
        QSharedPointer<rVect> v1 = tri->getV1();
        QSharedPointer<rVect> v2 = tri->getV2();
        QSharedPointer<rVect> v3 = tri->getV3();

        coordinateArray.append(v1->getX());
        coordinateArray.append(v1->getY());
        coordinateArray.append(v1->getZ());
        coordinateArray.append(v2->getX());
        coordinateArray.append(v2->getY());
        coordinateArray.append(v2->getZ());
        coordinateArray.append(v3->getX());
        coordinateArray.append(v3->getY());
        coordinateArray.append(v3->getZ());

        visibleTrianglesArray.append(tri->getVisible());
    }

    // QVector3D cameraPosition = openGLWidget->getCameraPosition();
    // rVect directVector = openGLWidget->QVector3DToRVect(cameraPosition);
    // QVector3D waveDirection(0.0f, 0.0f, -1.0f);
    // rVect directVector = openGLWidget->QVector3DToRVect(waveDirection);

    rVect directVector = calculateDirectVectorFromRotation();

    QJsonObject modelData;
    modelData["data"] = coordinateArray;
    modelData["visibleTriangles"] = visibleTrianglesArray;
    modelData["freqBand"] = freqBand;
    modelData["polarRadiation"] = polarRadiation;
    modelData["polarRecive"] = polarRecive;
    modelData["typeAngle"] = typeAngle;
    modelData["typeAzimut"] = typeAzimut;
    modelData["typeLength"] = typeLength;
    modelData["pplane"] = pplane;
    modelData["directVector"] = vectorToJson(QSharedPointer<rVect>::create(directVector));

    QJsonDocument doc(modelData);
    qDebug() << "Model data to be sent to server:" << doc.toJson(QJsonDocument::Indented);

    sendDataAfterAuthorization([this, modelData]() {
        triangleClient->sendModelData(modelData);
    });
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

    // Определение шага по оси X в зависимости от freqBand
    double stepX;
    switch (freqBand) {
    case 0: stepX = 1.5; break; // P-диапазон
    case 1: stepX = 0.5; break; // L-диапазон
    case 2: stepX = 0.5; break; // S-диапазон
    case 3: stepX = 0.5; break; // C-диапазон
    case 4: stepX = 0.25; break; // X-диапазон
    case 5: stepX = 0.125; break; // Ka-диапазон
    default: stepX = 1.0; break; // Значение по умолчанию
    }

    QVector<double> x;
    for (int i = 0; i < absEout.size(); ++i) {
        x.append(i * stepX);
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
    if (absEout2D.isEmpty() || normEout2D.isEmpty()) {
        logMessage("Нет данных для отображения.");
        return;
    }

    if (!portraitWindow) {
        portraitWindow = new PortraitWindow(this);
        portraitWindow->setAttribute(Qt::WA_DeleteOnClose);
    }

    portraitWindow->setData(absEout2D);
    portraitWindow->show();
}


void MainWindow::showGraph3D() {
    if (absEout2D.isEmpty() || normEout2D.isEmpty()) {
        // Сообщение об ошибке, если данных нет
        return;
    }

    graph3DWindow->clearData();
    graph3DWindow->setData(absEout2D, normEout2D);
    graph3DWindow->show();
}

// Реализация метода для открытия окна 3D Scatter Plot
void MainWindow::openScatterPlot3DWindow() {
    // Создаем новое окно для отображения Scatter Plot 3D
    ScatterPlot3DWindow *scatterPlot3DWindow = new ScatterPlot3DWindow(this);

    // Получаем вершины и индексы из OpenGL виджета для отображения объекта
    QVector<QVector3D> vertices = openGLWidget->getVertices();
    QVector<QVector<int>> indices = openGLWidget->getIndices();

    // Устанавливаем данные о вершинах и индексах в окно Scatter Plot 3D
    scatterPlot3DWindow->setData(vertices, indices);

    // Передаем уже полученные данные о рассеивании в Scatter Plot 3D
    scatterPlot3DWindow->setScatteringData(absEout, normEout);

    // Устанавливаем атрибут, чтобы окно закрылось и память освободилась при закрытии
    scatterPlot3DWindow->setAttribute(Qt::WA_DeleteOnClose);

    // Показываем окно Scatter Plot 3D
    scatterPlot3DWindow->show();
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

void MainWindow::saveLog() {
    QString defaultFileName = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss") + "_log.txt";
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить журнал действий", defaultFileName, "Text Files (*.txt)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << logDisplay->toPlainText();
        }
    }
}

void MainWindow::showAboutDialog() {
    QScopedPointer<AboutDialog> aboutDialog(new AboutDialog(this));
    aboutDialog->exec();
}
