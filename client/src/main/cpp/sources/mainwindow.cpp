#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "parser.h"
#include "raytracer.h"
#include "resultsdialog.h"
#include "aboutdialog.h"
#include "graphwindow.h"
#include "logindialog.h"
#include "portraitwindow.h"
#include "newprojectdialog.h"
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
#include <QScopedPointer>
#include <memory>
#include <QMessageBox>
#include <QCloseEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , openGLWidget(new OpenGLWidget(this))
    , parser(new Parser(this))
    , rayTracer(std::make_unique<RayTracer>())
    , triangleClient(nullptr) // Инициализируем указатель нулевым значением
    , hasNumericalData(false)
    , hasGraphData(false)
    , serverEnabled(false)
    , absEout()
    , normEout()
    , absEout2D()
    , normEout2D()
    , anglePortraitCheckBox(new QCheckBox("Угломестный", this))
    , azimuthPortraitCheckBox(new QCheckBox("Азимутальный", this))
    , rangePortraitCheckBox(new QCheckBox("Дальностный", this))
    , storedResults()
    , freqBandComboBox(new QComboBox(this))
    , pplaneCheckBox(new QCheckBox("Включить подстилающую поверхность", this))
    , radiationPolarizationComboBox(new QComboBox(this))
    , receivePolarizationComboBox(new QComboBox(this))
    , portraitWindow(new PortraitWindow(this))
    , isModified(false)
    , isDarkTheme(true)
    , resultsWatcher(new QFutureWatcher<void>(this))
{
    ui->setupUi(this);
    setCentralWidget(openGLWidget);

    // Создание меню-бара
    QMenuBar *menuBar = new QMenuBar(this);
    QMenu *fileMenu = new QMenu("Файл", this);

    QAction *openAction = new QAction(QIcon(":/load.png"), "Открыть", this);
    QAction *saveAction = new QAction(QIcon(":/download.png"), "Сохранить", this);
    QAction *exitAction = new QAction("Выход", this);

    QAction *closeAction = new QAction(QIcon(":/close.png"), "Закрыть", this);
    fileMenu->addAction(closeAction);
    connect(closeAction, &QAction::triggered, this, &MainWindow::closeModel);

    fileMenu->addAction(openAction);
    fileMenu->addAction(closeAction);
    fileMenu->addAction(saveAction);

    fileMenu->addSeparator();

    // Пункт меню "Новый проект"
    QAction *newProjectAction = new QAction(QIcon(":/new.png"), "Новый проект", this);
    fileMenu->addAction(newProjectAction);
    connect(newProjectAction, &QAction::triggered, this, &MainWindow::newProject);

    QAction *openProjectAction = new QAction(QIcon(":/open.png"), "Открыть проект", this);
    fileMenu->addAction(openProjectAction);
    connect(openProjectAction, &QAction::triggered, this, &MainWindow::openProject);

    // Пункт меню "Сохранить проект"
    QAction *saveProjectAction = new QAction(QIcon(":/download.png"), "Сохранить проект", this);
    fileMenu->addAction(saveProjectAction);
    connect(saveProjectAction, &QAction::triggered, this, &MainWindow::saveProject);

    fileMenu->addSeparator();
    fileMenu->addAction(exitAction);

    menuBar->addMenu(fileMenu);
    setMenuBar(menuBar);

    connect(openAction, &QAction::triggered, this, &MainWindow::loadModel);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveProject);
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);

    // Создание нового меню "Результаты"
    QMenu *resultsMenu = new QMenu("Результаты", this);

    // Пункт меню "Открыть числовые значения"
    openResultsAction = new QAction("Открыть числовые значения", this);
    openResultsAction->setEnabled(false);
    resultsMenu->addAction(openResultsAction);
    connect(openResultsAction, &QAction::triggered, this, &MainWindow::openResultsWindow);

    // Пункт меню "Открыть график"
    openGraphAction = new QAction("Открыть график", this);
    openGraphAction->setEnabled(false);
    resultsMenu->addAction(openGraphAction);
    connect(openGraphAction, &QAction::triggered, this, &MainWindow::openGraphWindow);

    // Пункт меню "Показать 2D портрет"
    showPortraitAction = new QAction("Показать 2D портрет", this);
    showPortraitAction->setEnabled(false);
    resultsMenu->addAction(showPortraitAction);
    connect(showPortraitAction, &QAction::triggered, this, &MainWindow::showPortrait);

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

    connect(openGLWidget, &OpenGLWidget::rotationChanged, this, [this](float x, float y, float z) {
        updateRotationX(x);
        updateRotationY(y);
        updateRotationZ(z);
    });

    setWindowTitle("ScatterModel3DClient");

    this->resize(1280, 720);

    // Соединения
    connect(triangleClient, &TriangleClient::resultsReceived, this, &MainWindow::handleDataReceived);

    connect(resultsWatcher, &QFutureWatcher<void>::finished, this, [this]() {
        logMessage("Обработка результатов завершена.");
        // Дополнительные действия после завершения
    });
    connect(parser, &Parser::fileParsed, this, [&](const QVector<QVector3D> &v, const QVector<QVector<int>> &t, const QVector<QSharedPointer<triangle>> &tri) {
        QVector3D observerPositionQVector = openGLWidget->getCameraPosition();
        rVect observerPosition = openGLWidget->QVector3DToRVect(observerPositionQVector);

        rayTracer->determineVisibility(tri, observerPosition);
        openGLWidget->setGeometry(v, t, tri);
    });

    // Создаем стек виджетов
    stackedWidget = new QStackedWidget(this);

    // Инициализируем виджеты для каждой вкладки
    parametersWidget = new QWidget;
    filteringWidget = new QWidget;
    serverWidget = new QWidget;

    // Настраиваем содержимое каждого виджета
    setupParametersWidget();
    setupFilteringWidget();
    setupServerWidget();

    // Добавляем виджеты в стек
    stackedWidget->addWidget(parametersWidget);
    stackedWidget->addWidget(filteringWidget);
    stackedWidget->addWidget(serverWidget);

    // Устанавливаем параметры как активную вкладку по умолчанию
    stackedWidget->setCurrentWidget(parametersWidget);

    // Создаем тулбар
    createToolBar();

    // Устанавливаем центральный виджет
    QWidget* centralWidget = new QWidget;
    QHBoxLayout* mainLayout = new QHBoxLayout(centralWidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // Устанавливаем фиксированную ширину для stackedWidget
    stackedWidget->setFixedWidth(400);  // Можно настроить нужную ширину

    // Сначала добавляем stackedWidget (левая часть)
    mainLayout->addWidget(stackedWidget);
    // Затем добавляем openGLWidget (правая часть)
    mainLayout->addWidget(openGLWidget);

    mainLayout->setStretch(0, 0);  // stackedWidget фиксированной ширины
    mainLayout->setStretch(1, 1);  // OpenGLWidget растягивается

    setCentralWidget(centralWidget);

    // Создание статусной строки
    statusBar()->showMessage("Нет активного проекта");

}

MainWindow::~MainWindow() {
    delete ui;
}

void MainWindow::createToolBar() {
    QToolBar *toolBar = addToolBar("Main Tools");
    toolBar->setMovable(false);
    toolBar->setIconSize(QSize(32, 32));

    // Создаем группу действий для взаимного исключения
    QActionGroup *actionGroup = new QActionGroup(this);
    actionGroup->setExclusive(true);

    // Создаем действия для каждой вкладки
    struct ToolButton {
        QString name;
        bool isChecked;
    };

    ToolButton buttons[] = {
        {"Параметры", true},
        {"Фильтрация", false},
        {"Сервер", false}
    };

    for (const auto& btn : buttons) {
        QAction* action = new QAction(btn.name, this);
        action->setCheckable(true);
        action->setChecked(btn.isChecked);
        actionGroup->addAction(action);
        toolBar->addAction(action);

        // Настраиваем внешний вид кнопки
        if (QToolButton* button = qobject_cast<QToolButton*>(
                toolBar->widgetForAction(action))) {
            button->setToolButtonStyle(Qt::ToolButtonTextOnly);
            button->setMinimumWidth(80);
        }
    }

    // Добавляем разделитель
    toolBar->addSeparator();

    // Добавляем кнопку журнала
    QAction* logAction = new QAction("Журнал", this);
    logAction->setCheckable(false);
    toolBar->addAction(logAction);

    if (QToolButton* button = qobject_cast<QToolButton*>(
            toolBar->widgetForAction(logAction))) {
        button->setToolButtonStyle(Qt::ToolButtonTextOnly);
        button->setMinimumWidth(80);
    }

    // Подключаем обработчик переключения
    connect(actionGroup, &QActionGroup::triggered, this, [this](QAction* action) {
        if (action->text() == "Параметры")
            stackedWidget->setCurrentWidget(parametersWidget);
        else if (action->text() == "Фильтрация")
            stackedWidget->setCurrentWidget(filteringWidget);
        else if (action->text() == "Сервер")
            stackedWidget->setCurrentWidget(serverWidget);
    });

    connect(logAction, &QAction::triggered, this, &MainWindow::showLogWindow);
}

void MainWindow::setupParametersWidget() {
    QVBoxLayout* layout = new QVBoxLayout(parametersWidget);
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);

    // Группа для поляризации
    QGroupBox *polarizationGroupBox = new QGroupBox("Поляризация", parametersWidget);
    QVBoxLayout *polarizationLayout = new QVBoxLayout(polarizationGroupBox);

    QLabel *radiationLabel = new QLabel("Излучения:", polarizationGroupBox);
    radiationPolarizationComboBox = new QComboBox(polarizationGroupBox);
    radiationPolarizationComboBox->addItems({"Горизонтальный", "Вертикальный", "Круговой"});

    QLabel *receiveLabel = new QLabel("Приёма:", polarizationGroupBox);
    receivePolarizationComboBox = new QComboBox(polarizationGroupBox);
    receivePolarizationComboBox->addItems({"Горизонтальный", "Вертикальный", "Круговой"});

    polarizationLayout->addWidget(radiationLabel);
    polarizationLayout->addWidget(radiationPolarizationComboBox);
    polarizationLayout->addWidget(receiveLabel);
    polarizationLayout->addWidget(receivePolarizationComboBox);

    // Группа для частот и поверхности
    QGroupBox *frequencyAndPlaneGroupBox = new QGroupBox("Параметры", parametersWidget);
    QVBoxLayout *frequencyAndPlaneLayout = new QVBoxLayout(frequencyAndPlaneGroupBox);

    freqBandComboBox = new QComboBox(frequencyAndPlaneGroupBox);
    freqBandComboBox->addItems({
        "P-диапазон (400-450 МГц)",
        "L-диапазон (1-1.5 ГГц)",
        "S-диапазон (2.75-3.15 ГГц)",
        "C-диапазон (5-5.5 ГГц)",
        "X-диапазон (9-10 ГГц)",
        "Ka-диапазон (36.5-38.5 ГГц)"
    });
    freqBandComboBox->setCurrentIndex(5);

    pplaneCheckBox = new QCheckBox("Включить подстилающую поверхность", frequencyAndPlaneGroupBox);
    gridCheckBox = new QCheckBox("Показать сетку", frequencyAndPlaneGroupBox);

    frequencyAndPlaneLayout->addWidget(new QLabel("Диапазон частот:"));
    frequencyAndPlaneLayout->addWidget(freqBandComboBox);
    frequencyAndPlaneLayout->addWidget(pplaneCheckBox);
    frequencyAndPlaneLayout->addWidget(gridCheckBox);

    // Группа для портретных типов
    QGroupBox *portraitTypeGroupBox = new QGroupBox("Портретные типы", parametersWidget);
    QVBoxLayout *portraitTypeLayout = new QVBoxLayout(portraitTypeGroupBox);

    anglePortraitCheckBox = new QCheckBox("Угломестный", portraitTypeGroupBox);
    azimuthPortraitCheckBox = new QCheckBox("Азимутальный", portraitTypeGroupBox);
    rangePortraitCheckBox = new QCheckBox("Дальностный", portraitTypeGroupBox);

    portraitTypeLayout->addWidget(anglePortraitCheckBox);
    portraitTypeLayout->addWidget(azimuthPortraitCheckBox);
    portraitTypeLayout->addWidget(rangePortraitCheckBox);

    // Группа для поворота
    QGroupBox *rotationGroupBox = new QGroupBox("Поворот", parametersWidget);
    QGridLayout *rotationLayout = new QGridLayout(rotationGroupBox);

    inputRotationX = new QDoubleSpinBox(rotationGroupBox);
    inputRotationX->setRange(-360, 360);
    inputRotationX->setSuffix(" °");

    inputRotationY = new QDoubleSpinBox(rotationGroupBox);
    inputRotationY->setRange(-360, 360);
    inputRotationY->setSuffix(" °");

    inputRotationZ = new QDoubleSpinBox(rotationGroupBox);
    inputRotationZ->setRange(-360, 360);
    inputRotationZ->setSuffix(" °");

    rotationLayout->addWidget(new QLabel("Поворот по X:"), 0, 0);
    rotationLayout->addWidget(inputRotationX, 0, 1);
    rotationLayout->addWidget(new QLabel("Поворот по Y:"), 1, 0);
    rotationLayout->addWidget(inputRotationY, 1, 1);
    rotationLayout->addWidget(new QLabel("Поворот по Z:"), 2, 0);
    rotationLayout->addWidget(inputRotationZ, 2, 1);

    QHBoxLayout *rotationButtonLayout = new QHBoxLayout();
    buttonApplyRotation = new QPushButton(QIcon(":/apply.png"), "Применить", rotationGroupBox);
    buttonResetRotation = new QPushButton(QIcon(":/reset.png"), "Сбросить", rotationGroupBox);
    rotationButtonLayout->addWidget(buttonApplyRotation);
    rotationButtonLayout->addWidget(buttonResetRotation);
    rotationLayout->addLayout(rotationButtonLayout, 3, 0, 1, 2);

    // Добавляем все группы в основной layout
    layout->addWidget(polarizationGroupBox);
    layout->addWidget(frequencyAndPlaneGroupBox);
    layout->addWidget(portraitTypeGroupBox);
    layout->addWidget(rotationGroupBox);
    layout->addStretch();

    // Переподключаем сигналы
    disconnect(buttonApplyRotation, nullptr, nullptr, nullptr);
    disconnect(buttonResetRotation, nullptr, nullptr, nullptr);
    disconnect(gridCheckBox, nullptr, nullptr, nullptr);
    disconnect(anglePortraitCheckBox, nullptr, nullptr, nullptr);
    disconnect(azimuthPortraitCheckBox, nullptr, nullptr, nullptr);
    disconnect(rangePortraitCheckBox, nullptr, nullptr, nullptr);
    disconnect(pplaneCheckBox, nullptr, nullptr, nullptr);

    connect(buttonApplyRotation, &QPushButton::clicked, this, &MainWindow::applyRotation);
    connect(buttonResetRotation, &QPushButton::clicked, this, &MainWindow::resetRotation);
    connect(gridCheckBox, &QCheckBox::stateChanged, this, &MainWindow::onGridCheckBoxStateChanged);
    connect(anglePortraitCheckBox, &QCheckBox::stateChanged, this, &MainWindow::onPortraitTypeChanged);
    connect(azimuthPortraitCheckBox, &QCheckBox::stateChanged, this, &MainWindow::onPortraitTypeChanged);
    connect(rangePortraitCheckBox, &QCheckBox::stateChanged, this, &MainWindow::onPortraitTypeChanged);
    connect(pplaneCheckBox, &QCheckBox::toggled, openGLWidget, &OpenGLWidget::setUnderlyingSurfaceVisible);
}

void MainWindow::setupFilteringWidget() {
    QVBoxLayout* layout = new QVBoxLayout(filteringWidget);
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);

    // Группа для фильтрации
    QGroupBox* filterGroupBox = new QGroupBox("Фильтрация", filteringWidget);
    QVBoxLayout* filterBoxLayout = new QVBoxLayout(filterGroupBox);

    // Дерево фильтрации
    filterTreeWidget = new QTreeWidget(filterGroupBox);
    filterTreeWidget->setHeaderHidden(true);
    filterTreeWidget->setAnimated(true);

    QTreeWidgetItem* filterRoot = new QTreeWidgetItem(filterTreeWidget, QStringList("Фильтрация"));
    filterRoot->setExpanded(true);

    shellStatsItem = new QTreeWidgetItem(filterRoot, QStringList("Оболочка: 0"));
    visibilityStatsItem = new QTreeWidgetItem(filterRoot, QStringList("Не в тени: 0"));

    // Кнопка фильтрации
    QPushButton* filterButton = new QPushButton("Выполнить фильтрацию", filterGroupBox);

    filterBoxLayout->addWidget(filterTreeWidget);
    filterBoxLayout->addWidget(filterButton);

    // Очищаем старые соединения
    disconnect(filterButton, nullptr, nullptr, nullptr);

    // Создаем новое соединение
    connect(filterButton, &QPushButton::clicked, this, &MainWindow::performFiltering);

    // Добавляем группы в основной layout
    layout->addWidget(filterGroupBox);
    layout->addStretch();
}

void MainWindow::setupServerWidget() {
    QVBoxLayout* layout = new QVBoxLayout(serverWidget);
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);

    // Группа для подключения к серверу
    QGroupBox *serverConnectionGroupBox = new QGroupBox("Подключение к серверу", serverWidget);
    QVBoxLayout *serverConnectionLayout = new QVBoxLayout(serverConnectionGroupBox);

    // Поле ввода адреса сервера
    serverAddressInput = new QLineEdit(serverConnectionGroupBox);
    serverAddressInput->setPlaceholderText("ws://serveraddress:port");
    serverAddressInput->setAlignment(Qt::AlignCenter);
    serverAddressInput->setFixedWidth(250);

    // Кнопки подключения/отключения
    QHBoxLayout *buttonLayout = new QHBoxLayout();
    connectButton = new QPushButton(QIcon(":/connect.png"), "Подключиться", serverConnectionGroupBox);
    QPushButton *disconnectButton = new QPushButton(QIcon(":/disconnect.png"), "Отключиться", serverConnectionGroupBox);

    // Устанавливаем фиксированные размеры для кнопок
    connectButton->setFixedSize(120, 30);
    disconnectButton->setFixedSize(120, 30);

    buttonLayout->addWidget(connectButton);
    buttonLayout->addWidget(disconnectButton);
    buttonLayout->setAlignment(Qt::AlignCenter);

    // Добавляем элементы в layout группы
    serverConnectionLayout->addWidget(serverAddressInput);
    serverConnectionLayout->addLayout(buttonLayout);
    serverConnectionLayout->setAlignment(Qt::AlignTop);

    // Очищаем старые соединения
    disconnect(connectButton, nullptr, nullptr, nullptr);
    disconnect(disconnectButton, nullptr, nullptr, nullptr);

    // Создаем новые соединения
    connect(connectButton, &QPushButton::clicked, this, &MainWindow::connectToServer);
    connect(disconnectButton, &QPushButton::clicked, this, &MainWindow::disconnectFromServer);

    // Добавляем группу подключения к серверу в основной layout
    layout->addWidget(serverConnectionGroupBox, 0, Qt::AlignTop);

    // Группа для выполнения расчета
    QGroupBox *calculationGroupBox = new QGroupBox("Выполнение расчета", serverWidget);
    QVBoxLayout *calculationLayout = new QVBoxLayout(calculationGroupBox);

    QPushButton *performCalculationButton = new QPushButton(QIcon(":/calculator.png"), "Выполнить расчёт", calculationGroupBox);
    performCalculationButton->setFixedSize(160, 30);
    calculationLayout->addWidget(performCalculationButton, 0, Qt::AlignCenter);

    connect(performCalculationButton, &QPushButton::clicked, this, &MainWindow::performCalculation);

    // Добавляем группу выполнения расчета в основной layout
    layout->addWidget(calculationGroupBox);
    layout->addStretch();
}

void MainWindow::showLogWindow()
{
    // Если окно журнала еще не создано, создаем его
    if (!logWindow) {
        logWindow = new QDialog(this);
        logWindow->setWindowTitle("Журнал действий");
        logWindow->setMinimumSize(500, 400);

        QVBoxLayout* layout = new QVBoxLayout(logWindow);

        // Переносим logDisplay в окно журнала
        if (!logDisplay) {
            logDisplay = new QTextEdit(logWindow);
            logDisplay->setReadOnly(true);
        }
        layout->addWidget(logDisplay);

        // Кнопка сохранения журнала
        QPushButton* saveButton = new QPushButton("Сохранить журнал", logWindow);
        connect(saveButton, &QPushButton::clicked, this, &MainWindow::saveLog);
        layout->addWidget(saveButton);

        // Кнопка закрытия
        QPushButton* closeButton = new QPushButton("Закрыть", logWindow);
        connect(closeButton, &QPushButton::clicked, logWindow, &QDialog::close);
        layout->addWidget(closeButton);

        // При закрытии окна не удаляем его
        logWindow->setAttribute(Qt::WA_DeleteOnClose, false);
    }

    logWindow->show();
    logWindow->raise();
    logWindow->activateWindow();
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

            // Сброс статистики фильтрации перед загрузкой новой модели
            QMetaObject::invokeMethod(this, [=]() {
                    MeshFilter::FilterStats emptyStats = {0, 0, 0};
                    updateFilterStats(emptyStats);
                }, Qt::QueuedConnection);

            // Парсим содержимое файла и загружаем данные в OpenGL виджет
            parser->readFromObjFile(fileName);

            // Проверка наличия треугольников
            QMetaObject::invokeMethod(this, [=]() {
                    if (openGLWidget->getTriangles().isEmpty()) {
                        logMessage("Ошибка: файл не содержит корректных данных объекта. Пожалуйста, загрузите корректный файл.");
                    } else {
                        logMessage("Файл успешно загружен.");
                        setModified(true); // Установка флага изменений
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
    // Копируем необходимые данные перед запуском в другом потоке
    bool angleChecked = anglePortraitCheckBox->isChecked();
    bool azimuthChecked = azimuthPortraitCheckBox->isChecked();
    bool rangeChecked = rangePortraitCheckBox->isChecked();

    QFuture<void> future = QtConcurrent::run([this, results, angleChecked, azimuthChecked, rangeChecked]() {
        processResults(results, angleChecked, azimuthChecked, rangeChecked);
    });

    resultsWatcher->setFuture(future);
}

void MainWindow::processResults(const QJsonObject &results, bool angleChecked, bool azimuthChecked, bool rangeChecked) {
    // Преобразование JSON-объекта в JSON-документ для дальнейшего использования
    QJsonDocument doc(results);
    QString localStoredResults = doc.toJson(QJsonDocument::Indented); // Локальная копия результатов

    // Извлечение массивов absEout и normEout из JSON-объекта результатов
    QJsonArray absEoutArray = results["absEout"].toArray();
    QJsonArray normEoutArray = results["normEout"].toArray();

    // Локальные копии векторов
    QVector<double> localAbsEout;
    QVector<double> localNormEout;
    QVector<QVector<double>> localAbsEout2D;
    QVector<QVector<double>> localNormEout2D;

    // Проверка, какие чекбоксы отмечены и установка соответствующей глубины вложенности
    if (angleChecked) {
        // Для угломестного типа также используется тройная вложенность
        extractValues(absEoutArray, localAbsEout, 3);
        extractValues(normEoutArray, localNormEout, 3);
    }
    if (azimuthChecked) {
        // Для азимутального типа используется тройная вложенность
        extractValues(absEoutArray, localAbsEout, 3);
        extractValues(normEoutArray, localNormEout, 3);
    }
    if (rangeChecked) {
        // Для дальностного типа требуется тройная вложенность
        extractValues(absEoutArray, localAbsEout, 3);
        extractValues(normEoutArray, localNormEout, 3);
    }

    // Извлечение двумерных массивов
    extract2DValues(absEoutArray, localAbsEout2D);
    extract2DValues(normEoutArray, localNormEout2D);

    // После обработки данных обновляем UI в главном потоке
    QMetaObject::invokeMethod(this, [=]() {
            storedResults = localStoredResults;
            absEout = localAbsEout;
            normEout = localNormEout;
            absEout2D = localAbsEout2D;
            normEout2D = localNormEout2D;
            logMessage("Числовые значения с сервера получены и сохранены.");

            // Вызов метода для обновления состояния кнопок
            updateMenuActions();

            setModified(true); // Установка флага изменений после получения данных
        }, Qt::QueuedConnection);
}

void MainWindow::updateRotationX(double x) {
    inputRotationX->blockSignals(true);
    inputRotationX->setValue(fmod(x, 360.0));  // Применяем модуль от 360
    inputRotationX->blockSignals(false);
}

void MainWindow::updateRotationY(double y) {
    inputRotationY->blockSignals(true);
    inputRotationY->setValue(fmod(y, 360.0));
    inputRotationY->blockSignals(false);
}

void MainWindow::updateRotationZ(double z) {
    inputRotationZ->blockSignals(true);
    inputRotationZ->setValue(fmod(z, 360.0));
    inputRotationZ->blockSignals(false);
}

void MainWindow::applyRotation() {
    float rotationX = inputRotationX->value();
    float rotationY = inputRotationY->value();
    float rotationZ = inputRotationZ->value();

    openGLWidget->setRotation(rotationX, rotationY, rotationZ);
    setModified(true); // Установка флага изменений
}

void MainWindow::resetRotation() {
    inputRotationX->setValue(0);
    inputRotationY->setValue(0);
    inputRotationZ->setValue(0);

    openGLWidget->setRotation(0, 0, 0);
    setModified(true); // Установка флага изменений
}

rVect MainWindow::calculateDirectVectorFromRotation() {
    float rotationX = inputRotationX->value();
    float rotationY = inputRotationY->value();
    float rotationZ = inputRotationZ->value();

    // Инвертируем порядок вращений, чтобы соответствовать порядку трансформаций в OpenGL
    QMatrix4x4 rotationMatrix;
    rotationMatrix.setToIdentity();
    rotationMatrix.rotate(rotationX, 1.0f, 0.0f, 0.0f);
    rotationMatrix.rotate(rotationY, 0.0f, 1.0f, 0.0f);
    rotationMatrix.rotate(rotationZ, 0.0f, 0.0f, 1.0f);

    // Вектор направления по умолчанию, направленный вдоль отрицательной оси Z
    QVector3D defaultDirection(0.0f, 0.0f, -1.0f);

    // Применяем матрицу вращения
    QVector3D transformedDirection = rotationMatrix.map(defaultDirection);

    // Нормализуем результат
    rVect directVector(transformedDirection.x(), transformedDirection.y(), transformedDirection.z());
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
        logMessage("Для начала подключитесь к серверу.");
        return;
    }

    if (!triangleClient->isAuthorized()) {
        authorizeClient();  // Вызываем метод авторизации, если не авторизованы
        QTimer::singleShot(2000, this, [this, sendDataFunc]() {
            if (triangleClient->isConnected() && triangleClient->isAuthorized()) {
                sendDataFunc();  // Отправляем данные после успешной авторизации
            } else {
                logMessage("Ошибка авторизации.");
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
        setModified(true); // Установка флага изменений после отправки данных
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

void MainWindow::logMessage(const QString& message)
{
    if (!logDisplay) {
        return;
    }

    QString displayedMessage = message;
    if (displayedMessage.length() > 500) {
        displayedMessage = displayedMessage.left(500) + "... [сообщение обрезано]";
    }

    QString timeStamp = QTime::currentTime().toString("HH:mm:ss");
    logDisplay->append(timeStamp + " - " + displayedMessage);

    // Если окно журнала существует, делаем его заметным
    if (logWindow && !logWindow->isVisible()) {
        logWindow->setWindowState((logWindow->windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    }
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
    if (!hasNumericalData) {
        logMessage("Нет доступных числовых значений.");
        return;
    }

    if (storedResults.isEmpty()) {
        logMessage("Нет доступных числовых значений.");
        return;
    }

    QScopedPointer<ResultsDialog> resultsDialog(new ResultsDialog(this));
    resultsDialog->setResults(storedResults);
    resultsDialog->exec();
}

void MainWindow::openGraphWindow() {
    if (!hasGraphData) {
        logMessage("Нет данных для графика.");
        return;
    }

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

void MainWindow::showPortrait() {
    if (!showPortraitAction->isEnabled()) {
        logMessage("Недостаточно данных или типов портретов для отображения 2D портрета.");
        return;
    }

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

bool MainWindow::saveProject() {
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить проект", "", "Файлы проекта (*.projjson);;Все файлы (*.*)");
    if (!fileName.isEmpty()) {
        // Сбор данных из OpenGLWidget и MainWindow
        currentProjectData.triangles = openGLWidget->getTriangles();
        currentProjectData.objectPosition = openGLWidget->getObjectPosition();
        currentProjectData.rotationX = inputRotationX->value();
        currentProjectData.rotationY = inputRotationY->value();
        currentProjectData.rotationZ = inputRotationZ->value();
        currentProjectData.scalingCoefficients = QVector3D(1.0f, 1.0f, 1.0f);
        currentProjectData.absEout = absEout;
        currentProjectData.normEout = normEout;
        currentProjectData.absEout2D = absEout2D;
        currentProjectData.normEout2D = normEout2D;
        // currentProjectData.storedResults уже обновляется в displayResults

        // Сохранение проекта
        if (ProjectSerializer::saveProject(fileName, currentProjectData)) {
            logMessage("Проект успешно сохранен: " + fileName);

            statusBar()->showMessage(QString("Проект: %1 | Рабочая директория: %2")
                                         .arg(currentProjectData.projectName.isEmpty() ? "Без названия" : currentProjectData.projectName)
                                         .arg(currentProjectData.workingDirectory.isEmpty() ? "Не задано" : currentProjectData.workingDirectory));

            setModified(false); // Сброс флага после успешного сохранения
            return true;
        } else {
            logMessage("Ошибка при сохранении проекта.");
            QMessageBox::critical(this, "Ошибка", "Не удалось сохранить проект.");
            return false;
        }
    }
    return false;
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

void MainWindow::onGridCheckBoxStateChanged(int state) {
    bool visible = (state == Qt::Checked);
    openGLWidget->setGridVisible(visible);
}

void MainWindow::openProject() {
    if (!maybeSave()) // Проверка на несохраненные изменения перед открытием нового проекта
        return;

    QString fileName = QFileDialog::getOpenFileName(this, "Открыть проект", "", "Файлы проекта (*.projjson);;Все файлы (*.*)");
    if (!fileName.isEmpty()) {
        ProjectData data;
        if (ProjectSerializer::loadProject(fileName, data)) {
            // Установка треугольников в OpenGLWidget
            openGLWidget->setTriangles(data.triangles);
            // Установка позиции объекта
            openGLWidget->setObjectPosition(data.objectPosition);
            // Установка поворотов
            inputRotationX->setValue(data.rotationX);
            inputRotationY->setValue(data.rotationY);
            inputRotationZ->setValue(data.rotationZ);
            openGLWidget->setRotation(data.rotationX, data.rotationY, data.rotationZ);
            // Установка коэффициентов масштабирования
            openGLWidget->setScalingCoefficients(data.scalingCoefficients);
            // Загрузка результатов расчёта
            absEout = data.absEout;
            normEout = data.normEout;
            absEout2D = data.absEout2D;
            normEout2D = data.normEout2D;
            storedResults = data.storedResults;

            currentProjectData.projectName = data.projectName;
            currentProjectData.workingDirectory = data.workingDirectory;

            // Обновление статусной строки
            statusBar()->showMessage(QString("Проект: %1 | Рабочая директория: %2")
                                         .arg(data.projectName.isEmpty() ? "Без названия" : data.projectName)
                                         .arg(data.workingDirectory.isEmpty() ? "Не задано" : data.workingDirectory));

            logMessage(QString("Проект успешно загружен: %1\nРабочая директория: %2")
                           .arg(data.projectName.isEmpty() ? "Без названия" : data.projectName)
                           .arg(data.workingDirectory.isEmpty() ? "Не задано" : data.workingDirectory));

            setModified(false); // Сброс флага после загрузки
        } else {
            logMessage("Ошибка при загрузке проекта.");
            QMessageBox::critical(this, "Ошибка", "Не удалось загрузить проект.");
        }
    }
}

void MainWindow::newProject() {
    if (!maybeSave()) // Проверка на несохраненные изменения перед созданием нового проекта
        return;

    // Создание и отображение диалога для нового проекта
    NewProjectDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        QString projectName = dialog.getProjectName();
        QString workingDirectory = dialog.getWorkingDirectory();

        // Проверка ввода
        if (projectName.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Название проекта не может быть пустым.");
            return;
        }

        if (workingDirectory.isEmpty()) {
            QMessageBox::warning(this, "Ошибка", "Рабочая директория не выбрана.");
            return;
        }

        // Установка значений в ProjectData
        currentProjectData.projectName = projectName;
        currentProjectData.workingDirectory = workingDirectory;

        // Обновление статусной строки
        statusBar()->showMessage(QString("Проект: %1 | Рабочая директория: %2")
                                     .arg(projectName)
                                     .arg(workingDirectory));

        // Вывод информации о проекте в журнал
        logMessage(QString("Создан новый проект: %1\nРабочая директория: %2")
                       .arg(projectName)
                       .arg(workingDirectory));
    } else {
        // Если пользователь отменил создание нового проекта
        return;
    }

    // Очистка OpenGL сцены
    openGLWidget->clearScene();

    // Сброс данных парсера
    parser->clearData();

    // Очистка результатов
    absEout.clear();
    normEout.clear();
    absEout2D.clear();
    normEout2D.clear();
    storedResults.clear();

    // Сброс вращения модели
    inputRotationX->setValue(0);
    inputRotationY->setValue(0);
    inputRotationZ->setValue(0);
    openGLWidget->setRotation(0, 0, 0);

    // Сброс позиции объекта
    openGLWidget->setObjectPosition(QVector3D(0.0f, 0.0f, 0.0f));

    // Сброс коэффициентов масштабирования
    openGLWidget->setScalingCoefficients(QVector3D(1.0f, 1.0f, 1.0f));

    // Сброс состояния чекбоксов и других элементов UI
    anglePortraitCheckBox->setChecked(false);
    azimuthPortraitCheckBox->setChecked(false);
    rangePortraitCheckBox->setChecked(false);
    pplaneCheckBox->setChecked(false);
    gridCheckBox->setChecked(false);
    freqBandComboBox->setCurrentIndex(5);
    radiationPolarizationComboBox->setCurrentIndex(0);
    receivePolarizationComboBox->setCurrentIndex(0);

    // Очистка журнала действий
    logDisplay->clear();

    // Вывод сообщения в журнал
    logMessage("Создан новый проект.");

    setModified(false); // Сброс флага после создания нового проекта
}

void MainWindow::handleDataReceived(const QJsonObject &results) {
    // Обработка результатов и обновление внутренних состояний
    displayResults(results);
    setModified(true); // Установка флага изменений после получения данных
}

void MainWindow::updateMenuActions() {
    // Логика для "Открыть числовые значения"
    if (!absEout.isEmpty() && !normEout.isEmpty()) {
        hasNumericalData = true;
        openResultsAction->setEnabled(true);
    } else {
        hasNumericalData = false;
        openResultsAction->setEnabled(false);
    }

    // Логика для "Открыть график"
    if (!absEout.isEmpty() && !normEout.isEmpty()) {
        hasGraphData = true;
        openGraphAction->setEnabled(true);
    } else {
        hasGraphData = false;
        openGraphAction->setEnabled(false);
    }

    // Логика для "Показать 2D портрет"
    int selectedPortraits = 0;
    if (anglePortraitCheckBox->isChecked()) selectedPortraits++;
    if (azimuthPortraitCheckBox->isChecked()) selectedPortraits++;
    if (rangePortraitCheckBox->isChecked()) selectedPortraits++;

    if (selectedPortraits == 2 && !absEout2D.isEmpty() && !normEout2D.isEmpty()) {
        showPortraitAction->setEnabled(true);
    } else {
        showPortraitAction->setEnabled(false);
    }
}

void MainWindow::onPortraitTypeChanged() {
    updateMenuActions();
}

// Метод для установки флага isModified
void MainWindow::setModified(bool modified) {
    if (isModified != modified) {
        isModified = modified;
        // Обновление заголовка окна, чтобы отразить состояние модификации
        if (isModified) {
            setWindowTitle(QString("%1* - ScatterModel3DClient").arg(currentProjectData.projectName.isEmpty() ? "Без названия" : currentProjectData.projectName));
        } else {
            setWindowTitle(QString("%1 - ScatterModel3DClient").arg(currentProjectData.projectName.isEmpty() ? "Без названия" : currentProjectData.projectName));
        }
    }
}

// Метод для запроса сохранения изменений
bool MainWindow::maybeSave() {
    if (!isModified)
        return true;

    QMessageBox::StandardButton ret;
    ret = QMessageBox::warning(this, tr("ScatterModel3DClient"),
                               tr("Текущий проект был изменен.\nХотите сохранить изменения?"),
                               QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (ret == QMessageBox::Save) {
        return saveProject();
    } else if (ret == QMessageBox::Cancel) {
        return false;
    }
    return true;
}

// Переопределение события закрытия окна
void MainWindow::closeEvent(QCloseEvent *event) {
    if (maybeSave()) {
        event->accept();
    } else {
        event->ignore();
    }
}

void MainWindow::performFiltering() {
    // if (triangles.isEmpty()) {
    //     logMessage("Ошибка: не загружен 3D объект для фильтрации.");
    //     return;
    // }

    QVector<QSharedPointer<triangle>> filteredTriangles = openGLWidget->getTriangles();
    MeshFilter::FilterStats stats = meshFilter.filterMesh(filteredTriangles);

    // Обновление OpenGL виджета с отфильтрованными треугольниками
    openGLWidget->setTriangles(filteredTriangles);

    // Обновление статистики
    updateFilterStats(stats);

    logMessage(QString("Фильтрация завершена. Всего треугольников: %1, "
                       "Оболочка: %2, Видимые: %3")
                   .arg(stats.totalTriangles)
                   .arg(stats.shellTriangles)
                   .arg(stats.visibleTriangles));

    setModified(true);
}

void MainWindow::updateFilterStats(const MeshFilter::FilterStats& stats) {
    if (shellStatsItem && visibilityStatsItem) {
        shellStatsItem->setText(0, QString("Оболочка: %1").arg(stats.shellTriangles));
        visibilityStatsItem->setText(0, QString("Не в тени: %1").arg(stats.visibleTriangles));
    }
}

void MainWindow::closeModel() {
    if (openGLWidget) {
        openGLWidget->clearScene();  // Очищаем сцену OpenGL
        parser->clearData();         // Очищаем данные парсера

        // Сброс статистики фильтрации при закрытии модели
        MeshFilter::FilterStats emptyStats = {0, 0, 0};
        updateFilterStats(emptyStats);

        logMessage("3D модель закрыта");
        setModified(true);          // Устанавливаем флаг изменений
    }
}
