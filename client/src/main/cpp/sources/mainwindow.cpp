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
#include <QGroupBox>

MainWindow::MainWindow(ConnectionManager* connectionManager, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , openGLWidget(new OpenGLWidget(this))
    , hasNumericalData(false)
    , hasGraphData(false)
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
    , patternDiagramWindow(nullptr)
    , m_connectionManager(connectionManager)
    , m_modelController(new ModelController(this))
{
    ui->setupUi(this);
    setCentralWidget(openGLWidget);

    // Регистрация MainWindow как наблюдателя ConnectionManager
    if (m_connectionManager) {
        m_connectionManager->registerObserver(this);
    }

    // Регистрируем MainWindow как наблюдателя ModelController
    m_modelController->registerObserver(this);
    // Устанавливаем OpenGLWidget в ModelController
    m_modelController->setOpenGLWidget(openGLWidget);

    // Соединяем сигналы ModelController с слотами MainWindow
    connect(m_modelController.get(), &ModelController::logMessage, this, &MainWindow::logMessage);
    connect(m_modelController.get(), &ModelController::notificationRequested, this,
            [this](const QString &message, int type) {
                showNotification(message, static_cast<Notification::Type>(type));
            });
    connect(m_modelController.get(), &ModelController::rotationChanged, this,
            [this](float x, float y, float z) {
                updateRotationX(x);
                updateRotationY(y);
                updateRotationZ(z);
            });

    // Создание меню-бара
    QMenuBar *menuBar = new QMenuBar(this);
    QMenu *fileMenu = new QMenu("Файл", this);

    QAction *openAction = new QAction(QIcon(":/load.png"), "Открыть", this);
    openAction->setToolTip("Открыть 3D модель");
    QAction *saveAction = new QAction(QIcon(":/download.png"), "Сохранить", this);
    QAction *exitAction = new QAction("Выход", this);

    QAction *closeAction = new QAction(QIcon(":/close.png"), "Закрыть", this);
    closeAction->setToolTip("Закрыть текущую 3D модель");
    closeAction->setEnabled(false); // Изначально кнопка неактивна
    fileMenu->addAction(closeAction);
    connect(closeAction, &QAction::triggered, this, &MainWindow::handleCloseModel);

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

    connect(openAction, &QAction::triggered, this, &MainWindow::handleLoadModel);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveProject);
    connect(exitAction, &QAction::triggered, this, &MainWindow::close);

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

    logDisplay = new QTextEdit(this);
    logDisplay->setReadOnly(true);
    logDisplay->hide();

    setWindowTitle("ScatterModel3DClient");

    this->resize(1280, 720);

    // Соединения
    connect(resultsWatcher, &QFutureWatcher<void>::finished, this, [this]() {
        logMessage("Обработка результатов завершена.");
        // Дополнительные действия после завершения
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
    delete NotificationManager::instance();
    if (patternDiagramWindow) {
        delete patternDiagramWindow;
    }
    if (m_connectionManager) {
        m_connectionManager->unregisterObserver(this);
    }
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

    // Добавляем кнопку результатов
    QToolButton* resultsButton = new QToolButton;
    resultsButton->setIcon(QIcon(":/results.png"));
    resultsButton->setText("Результаты");
    resultsButton->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
    resultsButton->setPopupMode(QToolButton::InstantPopup);

    QMenu* resultsMenu = new QMenu(resultsButton);

    // Действия для разных типов результатов
    openResultsAction = resultsMenu->addAction(QIcon(":/numerical.png"), "Числовые значения");
    openGraphAction = resultsMenu->addAction(QIcon(":/graph1d.png"), "Одномерный график");
    showPortraitAction = resultsMenu->addAction(QIcon(":/graph2d.png"), "Двумерный портрет");
    QAction* showPatternDiagramAction = resultsMenu->addAction(QIcon(":/diagram.png"), "Диаграмма направленности");

    // Изначально все действия неактивны
    openResultsAction->setEnabled(false);
    openGraphAction->setEnabled(false);
    showPortraitAction->setEnabled(false);
    showPatternDiagramAction->setEnabled(true);

    resultsButton->setMenu(resultsMenu);
    toolBar->addWidget(resultsButton);

    // Подключаем сигналы для результатов
    connect(openResultsAction, &QAction::triggered, this, &MainWindow::openResultsWindow);
    connect(openGraphAction, &QAction::triggered, this, &MainWindow::openGraphWindow);
    connect(showPortraitAction, &QAction::triggered, this, &MainWindow::showPortrait);
    connect(showPatternDiagramAction, &QAction::triggered, this, &MainWindow::showPatternDiagram);

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

    // Подключаем обработчики
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

    // Добавляем слайдер и метку для настройки прозрачности поверхности
    QHBoxLayout *surfaceAlphaLayout = new QHBoxLayout();
    QLabel *surfaceAlphaLabel = new QLabel("Прозрачность поверхности:", frequencyAndPlaneGroupBox);
    QSlider *surfaceAlphaSlider = new QSlider(Qt::Horizontal, frequencyAndPlaneGroupBox);
    surfaceAlphaSlider->setRange(10, 80);  // Значения от 0.1 до 0.8
    surfaceAlphaSlider->setValue(static_cast<int>(openGLWidget->getSurfaceAlpha() * 100));
    surfaceAlphaSlider->setEnabled(pplaneCheckBox->isChecked());

    surfaceAlphaLayout->addWidget(surfaceAlphaLabel);
    surfaceAlphaLayout->addWidget(surfaceAlphaSlider);

    gridCheckBox = new QCheckBox("Показать сетку", frequencyAndPlaneGroupBox);
    gridCheckBox->setChecked(true);

    frequencyAndPlaneLayout->addWidget(new QLabel("Диапазон частот:"));
    frequencyAndPlaneLayout->addWidget(freqBandComboBox);
    frequencyAndPlaneLayout->addWidget(pplaneCheckBox);
    frequencyAndPlaneLayout->addLayout(surfaceAlphaLayout);
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

    buttonResetScale = new QPushButton(QIcon(":/reset.png"), "Сбросить масштаб", rotationGroupBox);


    rotationButtonLayout->addWidget(buttonApplyRotation);
    rotationButtonLayout->addWidget(buttonResetRotation);
    rotationButtonLayout->addWidget(buttonResetScale);
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

    // Соединяем слайдер прозрачности с OpenGLWidget
    connect(surfaceAlphaSlider, &QSlider::valueChanged, this, [this, surfaceAlphaSlider](int value) {
        float alpha = value / 100.0f;
        openGLWidget->setSurfaceAlpha(alpha);
    });

    // Активация/деактивация слайдера прозрачности при изменении состояния чекбокса
    connect(pplaneCheckBox, &QCheckBox::toggled, surfaceAlphaSlider, &QSlider::setEnabled);
    connect(buttonApplyRotation, &QPushButton::clicked, this, &MainWindow::handleApplyRotation);
    connect(buttonResetRotation, &QPushButton::clicked, this, &MainWindow::handleResetRotation);
    connect(buttonResetScale, &QPushButton::clicked, this, &MainWindow::handleResetScale);
    connect(gridCheckBox, &QCheckBox::stateChanged, this, &MainWindow::onGridCheckBoxStateChanged);
    connect(anglePortraitCheckBox, &QCheckBox::stateChanged, this, &MainWindow::onPortraitTypeChanged);
    connect(azimuthPortraitCheckBox, &QCheckBox::stateChanged, this, &MainWindow::onPortraitTypeChanged);
    connect(rangePortraitCheckBox, &QCheckBox::stateChanged, this, &MainWindow::onPortraitTypeChanged);
    connect(pplaneCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        openGLWidget->setUnderlyingSurfaceVisible(checked);

        // Также обновляем флаг в объекте данных проекта
        currentProjectData.showUnderlyingSurface = checked;
        setModified(true); // Отмечаем, что проект изменен
    });
}

void MainWindow::setupFilteringWidget() {
    QVBoxLayout* layout = new QVBoxLayout(filteringWidget);
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);

    QString statsStyle = "font-weight: bold;";

    // Группа для информации о модели
    QGroupBox* modelInfoGroup = new QGroupBox("Информация о модели", filteringWidget);
    QGridLayout* modelInfoLayout = new QGridLayout(modelInfoGroup);

    // Создаем метки для отображения информации
    QLabel* fileLabel = new QLabel("Файл:", modelInfoGroup);
    QLabel* verticesLabel = new QLabel("Количество вершин:", modelInfoGroup);
    QLabel* trianglesLabel = new QLabel("Количество треугольников:", modelInfoGroup);

    modelFileNameLabel = new QLabel("не загружен", modelInfoGroup);
    totalNodesLabel = new QLabel("0", modelInfoGroup);
    totalTrianglesLabel = new QLabel("0", modelInfoGroup);

    modelFileNameLabel->setStyleSheet(statsStyle);
    totalNodesLabel->setStyleSheet(statsStyle);
    totalTrianglesLabel->setStyleSheet(statsStyle);

    modelInfoLayout->addWidget(fileLabel, 0, 0);
    modelInfoLayout->addWidget(modelFileNameLabel, 0, 1);
    modelInfoLayout->addWidget(verticesLabel, 1, 0);
    modelInfoLayout->addWidget(totalNodesLabel, 1, 1);
    modelInfoLayout->addWidget(trianglesLabel, 2, 0);
    modelInfoLayout->addWidget(totalTrianglesLabel, 2, 1);

    QPushButton* showAllTrianglesButton = new QPushButton("Вернуть исходное состояние объекта", modelInfoGroup);
    showAllTrianglesButton->setToolTip("Восстановить все треугольники модели, включая скрытые.");
    modelInfoLayout->addWidget(showAllTrianglesButton, 3, 0, 1, 2);

    layout->addWidget(modelInfoGroup);

    // Группа для управления видимостью объекта
    QGroupBox* visibilityGroupBox = new QGroupBox("Управление видимостью объекта", filteringWidget);
    QGridLayout* visibilityLayout = new QGridLayout(visibilityGroupBox);

    QLabel* visibleCountLabel = new QLabel("Не в тени:", visibilityGroupBox);
    QLabel* removedShadowLabel = new QLabel("Скрыто (тени):", visibilityGroupBox);

    visibleCountDisplay = new QLabel("0", visibilityGroupBox);
    removedShadowDisplay = new QLabel("0", visibilityGroupBox);

    visibleCountDisplay->setStyleSheet(statsStyle);
    removedShadowDisplay->setStyleSheet(statsStyle);

    visibilityLayout->addWidget(visibleCountLabel, 0, 0);
    visibilityLayout->addWidget(visibleCountDisplay, 0, 1);
    visibilityLayout->addWidget(removedShadowLabel, 1, 0);
    visibilityLayout->addWidget(removedShadowDisplay, 1, 1);

    QPushButton* toggleShellButton = new QPushButton("Скрыть теневые треугольники", visibilityGroupBox);
    visibilityLayout->addWidget(toggleShellButton, 2, 0, 1, 2);

    layout->addWidget(visibilityGroupBox);

    // Группа для анализа и оптимизации
    QGroupBox* analysisGroupBox = new QGroupBox("Анализ и оптимизация", filteringWidget);
    QGridLayout* analysisLayout = new QGridLayout(analysisGroupBox);

    QLabel* shellCountLabel = new QLabel("Оболочка:", analysisGroupBox);
    QLabel* removedShellLabel = new QLabel("Удалено (оптимизация):", analysisGroupBox);

    shellCountDisplay = new QLabel("0", analysisGroupBox);
    removedShellDisplay = new QLabel("0", analysisGroupBox);

    shellCountDisplay->setStyleSheet(statsStyle);
    removedShellDisplay->setStyleSheet(statsStyle);

    analysisLayout->addWidget(shellCountLabel, 0, 0);
    analysisLayout->addWidget(shellCountDisplay, 0, 1);
    analysisLayout->addWidget(removedShellLabel, 1, 0);
    analysisLayout->addWidget(removedShellDisplay, 1, 1);

    QPushButton* optimizeButton = new QPushButton("Оптимизировать структуру объекта", analysisGroupBox);
    optimizeButton->setToolTip("Анализ и удаление внутренних треугольников для оптимизации расчётов");
    analysisLayout->addWidget(optimizeButton, 2, 0, 1, 2);

    layout->addWidget(analysisGroupBox);
    layout->addStretch();

    // Подключение сигналов
    connect(toggleShellButton, &QPushButton::clicked, this, &MainWindow::handleToggleShadowTriangles);
    connect(showAllTrianglesButton, &QPushButton::clicked, this, [this]() {
        m_modelController->resetToOriginalModel();
    });
    connect(optimizeButton, &QPushButton::clicked, this, &MainWindow::handleFilterModel);
}

void MainWindow::setupServerWidget() {
    QVBoxLayout* layout = new QVBoxLayout(serverWidget);
    layout->setSpacing(10);
    layout->setContentsMargins(10, 10, 10, 10);

    QString statsStyle = "font-weight: bold;";

    // Группа для статуса подключения
    QGroupBox* statusGroupBox = new QGroupBox("Статус подключения", serverWidget);
    QGridLayout* statusLayout = new QGridLayout(statusGroupBox);
    statusLayout->setColumnStretch(1, 1);

    // Индикатор статуса
    QLabel* statusTextLabel = new QLabel("Статус:", statusGroupBox);
    QLabel* statusLabel = new QLabel("Не подключен", statusGroupBox);
    statusLabel->setStyleSheet(statsStyle + "color: #FF4444;");

    statusLayout->addWidget(statusTextLabel, 0, 0);
    statusLayout->addWidget(statusLabel, 0, 1, Qt::AlignLeft);

    // Группа для подключения к серверу
    QGroupBox* serverConnectionGroupBox = new QGroupBox("Настройки подключения", serverWidget);
    QGridLayout* serverConnectionLayout = new QGridLayout(serverConnectionGroupBox);

    // Поле ввода адреса сервера с меткой
    QLabel* addressLabel = new QLabel("Адрес сервера:", serverConnectionGroupBox);
    serverAddressInput = new QLineEdit(serverConnectionGroupBox);
    serverAddressInput->setPlaceholderText("ws://serveraddress:port");
    serverAddressInput->setAlignment(Qt::AlignCenter);

    // Кнопки подключения/отключения
    connectButton = new QPushButton(QIcon(":/connect.png"), "Подключиться", serverConnectionGroupBox);
    QPushButton* disconnectButton = new QPushButton(QIcon(":/disconnect.png"), "Отключиться", serverConnectionGroupBox);

    // Добавляем элементы в layout группы подключения
    serverConnectionLayout->addWidget(addressLabel, 0, 0, 1, 2);
    serverConnectionLayout->addWidget(serverAddressInput, 1, 0, 1, 2);
    serverConnectionLayout->addWidget(connectButton, 2, 0);
    serverConnectionLayout->addWidget(disconnectButton, 2, 1);

    // Группа для управления расчётами
    QGroupBox* calculationControlGroupBox = new QGroupBox("Управление расчётами", serverWidget);
    QGridLayout* calculationControlLayout = new QGridLayout(calculationControlGroupBox);

    // Индикатор прогресса и метку
    QLabel* progressLabel = new QLabel("Прогресс расчёта:", calculationControlGroupBox);
    progressBar = new QProgressBar(calculationControlGroupBox);
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->setValue(0);
    progressBar->setFormat("%p%");
    progressBar->setTextVisible(true);
    progressBar->setStyleSheet("QProgressBar { border: none; background: transparent; } QProgressBar::chunk { background: none; }");

    // Кнопки управления расчётом
    QPushButton* performCalculationButton = new QPushButton(QIcon(":/calculator.png"), "Выполнить расчёт", calculationControlGroupBox);
    abortCalculationButton = new QPushButton(QIcon(":/stop-calculator-button.png"), "Прервать расчёт", calculationControlGroupBox);
    abortCalculationButton->setEnabled(false);

    calculationControlLayout->addWidget(progressLabel, 0, 0, Qt::AlignVCenter);
    calculationControlLayout->addWidget(progressBar, 0, 1, Qt::AlignVCenter);
    calculationControlLayout->addWidget(performCalculationButton, 1, 0, 1, 2);
    calculationControlLayout->addWidget(abortCalculationButton, 2, 0, 1, 2);

    // Добавляем все группы в основной layout
    layout->addWidget(statusGroupBox);
    layout->addWidget(serverConnectionGroupBox);
    layout->addWidget(calculationControlGroupBox);
    layout->addStretch();

    // Очищаем старые соединения
    disconnect(connectButton, nullptr, nullptr, nullptr);
    disconnect(disconnectButton, nullptr, nullptr, nullptr);
    disconnect(performCalculationButton, nullptr, nullptr, nullptr);

    // Создаем новые соединения с использованием ConnectionManager
    connect(connectButton, &QPushButton::clicked, this, [this]() {
        if (m_connectionManager) {
            m_connectionManager->connectToServer(serverAddressInput->text().trimmed());
        }
    });

    connect(disconnectButton, &QPushButton::clicked, this, [this]() {
        if (m_connectionManager) {
            m_connectionManager->disconnectFromServer();
        }
    });

    connect(abortCalculationButton, &QPushButton::clicked, this, [this]() {
        if (m_connectionManager) {
            m_connectionManager->abortCalculation();
        }
    });

    connect(performCalculationButton, &QPushButton::clicked, this, &MainWindow::performCalculation);

    // Обновление статуса при подключении/отключении
    connect(this, &MainWindow::connectionStatusChanged, [statusLabel](bool connected) {
        if (connected) {
            statusLabel->setText("Подключен");
            statusLabel->setStyleSheet("font-weight: bold; color: #44FF44;");
        } else {
            statusLabel->setText("Не подключен");
            statusLabel->setStyleSheet("font-weight: bold; color: #FF4444;");
        }
    });
}

void MainWindow::showLogWindow()
{
    // Если окно журнала еще не создано, создаем его
    if (!logWindow) {
        logWindow = new QDialog(this);
        logWindow->setWindowTitle("Журнал действий");
        logWindow->setMinimumSize(500, 400);

        // Создаем новый QTextEdit специально для окна журнала
        if (!logDisplay) {
            logDisplay = new QTextEdit();
            logDisplay->setReadOnly(true);
        }

        QVBoxLayout* layout = new QVBoxLayout(logWindow);
        logDisplay->show();
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

PortraitDimension MainWindow::detectPortraitDimension(const QJsonArray &array) {
    if (array.isEmpty())
        return PortraitDimension::Undefined;

    int dim1Size = array.size();
    int dim2Size = 0, dim3Size = 0;

    if (!array.at(0).isArray())
        return PortraitDimension::Undefined;

    QJsonArray dim2Array = array.at(0).toArray();
    dim2Size = dim2Array.size();

    if (!dim2Array.isEmpty() && dim2Array.at(0).isArray()) {
        QJsonArray dim3Array = dim2Array.at(0).toArray();
        dim3Size = dim3Array.size();
    }

    qDebug() << "Data structure detected: [" << dim1Size << "][" << dim2Size << "][" << dim3Size << "]";

    // Проверяем содержимое, чтобы определить реальную размерность данных
    QVector<int> dim1NonEmptyCount = {0};
    QVector<int> dim2NonEmptyCount;
    QVector<int> dim3NonEmptyCount;

    for (int i = 0; i < dim1Size; ++i) {
        if (!array.at(i).isArray()) continue;

        QJsonArray arr1 = array.at(i).toArray();
        dim1NonEmptyCount[0]++;

        for (int j = 0; j < arr1.size(); ++j) {
            if (!arr1.at(j).isArray()) continue;

            if (i == 0) dim2NonEmptyCount.append(0);
            dim2NonEmptyCount[j]++;

            QJsonArray arr2 = arr1.at(j).toArray();
            for (int k = 0; k < arr2.size(); ++k) {
                if (i == 0 && j == 0) dim3NonEmptyCount.append(0);
                if (!arr2.at(k).isArray() || !arr2.at(k).toArray().isEmpty()) {
                    dim3NonEmptyCount[k]++;
                }
            }
        }
    }

    // Подсчитываем эффективные размерности
    int effectiveDim1 = dim1NonEmptyCount[0];
    int effectiveDim2 = 0;
    for (int count : dim2NonEmptyCount) if (count > 0) effectiveDim2++;
    int effectiveDim3 = 0;
    for (int count : dim3NonEmptyCount) if (count > 0) effectiveDim3++;

    qDebug() << "Effective dimensions: [" << effectiveDim1 << "][" << effectiveDim2 << "][" << effectiveDim3 << "]";

    // Дальностный: [1][n][1], где n - количество дальностных отсчетов
    if (dim1Size == 1 && dim2Size > 1 && dim3Size == 1)
        return PortraitDimension::Range;

    // Азимутальный: [1][1][n], где n - количество азимутальных отсчетов
    if (dim1Size == 1 && dim2Size == 1 && dim3Size > 1)
        return PortraitDimension::Azimuth;

    // Угломестный: [n][1][1]
    if (dim1Size > 1 && dim2Size == 1 && dim3Size == 1)
        return PortraitDimension::Elevation;

    // Комбинированные типы:
    // Угломестный+Дальностный: [n][m][1] - где n - количество угломестных отсчетов, m - дальностных
    if (dim1Size > 1 && dim2Size > 1 && dim3Size == 1)
        return PortraitDimension::ElevationRange;

    // Азимутальный+Дальностный: [1][n][m] - где n - количество азимутальных отсчетов, m - дальностных
    if (dim1Size == 1 && dim2Size > 1 && dim3Size > 1)
        return PortraitDimension::AzimuthRange;

    // Угломестный+Азимутальный: [n][1][m] - где n - количество угломестных отсчетов, m - азимутальных
    if (dim1Size > 1 && dim2Size == 1 && dim3Size > 1)
        return PortraitDimension::ElevationAzimuth;

    // Угломестный+Азимутальный+Дальностный: [n][m][k]
    if (dim1Size > 1 && dim2Size > 1 && dim3Size > 1)
        return PortraitDimension::ThreeDimensional;

    // По умолчанию считаем одномерным
    return PortraitDimension::Undefined;
}

void MainWindow::prepare2DDataForDisplay(const PortraitData &data,
                                         bool angleChecked, bool azimuthChecked, bool rangeChecked,
                                         QVector<double> &data1D, QVector<QVector<double>> &data2D) {
    data1D.clear();
    data2D.clear();

    qDebug() << "Dimension in prepare2DDataForDisplay:" << static_cast<int>(data.dimension);
    qDebug() << "3D Data size:" << data.data3D.size();

    if (data.data3D.isEmpty()) {
        qDebug() << "No 3D data to process!";
        return;
    }

    switch (data.dimension) {
    case PortraitDimension::Range: // [1][n][1]
    {
        if (rangeChecked) {
            QVector<double> rangeData;
            // Извлекаем данные из формата [1][n][1]
            if (!data.data3D.isEmpty() && !data.data3D[0].isEmpty()) {
                for (int i = 0; i < data.data3D[0].size(); ++i) {
                    if (!data.data3D[0][i].isEmpty()) {
                        rangeData.append(data.data3D[0][i][0]);
                    }
                }
                data1D = rangeData;
                data2D.append(rangeData);
                qDebug() << "Range data processed, 1D size:" << data1D.size();
            }
        }
    }
    break;

    case PortraitDimension::Azimuth: // [1][1][n]
    {
        if (azimuthChecked) {
            // Извлекаем данные из формата [1][1][n]
            if (!data.data3D.isEmpty() && !data.data3D[0].isEmpty() && !data.data3D[0][0].isEmpty()) {
                data1D = data.data3D[0][0];
                data2D.append(data1D);
                qDebug() << "Azimuth data processed, 1D size:" << data1D.size();
            }
        }
    }
    break;

    case PortraitDimension::Elevation: // [n][1][1]
    {
        if (angleChecked) {
            QVector<double> elevationData;
            for (int i = 0; i < data.data3D.size(); ++i) {
                if (!data.data3D[i].isEmpty() && !data.data3D[i][0].isEmpty()) {
                    elevationData.append(data.data3D[i][0][0]);
                }
            }
            data1D = elevationData;
            data2D.append(elevationData);
            qDebug() << "Elevation data processed, 1D size:" << data1D.size();
        }
    }
    break;

    case PortraitDimension::ElevationRange: // [n][m][1] - Угломестный+Дальностный
    {
        if (angleChecked && rangeChecked) {
            for (int i = 0; i < data.data3D.size(); ++i) {
                QVector<double> row;
                for (int j = 0; j < data.data3D[i].size(); ++j) {
                    if (!data.data3D[i][j].isEmpty()) {
                        row.append(data.data3D[i][j][0]);
                    }
                }
                if (!row.isEmpty()) {
                    data2D.append(row);
                }
            }
            qDebug() << "ElevationRange data processed, 2D size:" << data2D.size()
                     << "x" << (data2D.isEmpty() ? 0 : data2D[0].size());
        }
    }
    break;

    case PortraitDimension::AzimuthRange: // [1][n][m] - Азимутальный+Дальностный
    {
        if (azimuthChecked && rangeChecked) {
            if (!data.data3D.isEmpty()) {
                for (int i = 0; i < data.data3D[0].size(); ++i) {
                    data2D.append(data.data3D[0][i]);
                }
                qDebug() << "AzimuthRange data processed, 2D size:" << data2D.size()
                         << "x" << (data2D.isEmpty() ? 0 : data2D[0].size());
            }
        }
    }
    break;

    case PortraitDimension::ElevationAzimuth: // [n][1][m] - Угломестный+Азимутальный
    {
        if (angleChecked && azimuthChecked) {
            // Создаем 2D массив, где строки - угломестные значения, столбцы - азимутальные
            for (int i = 0; i < data.data3D.size(); ++i) {
                if (!data.data3D[i].isEmpty() && !data.data3D[i][0].isEmpty()) {
                    data2D.append(data.data3D[i][0]);
                }
            }
            qDebug() << "ElevationAzimuth data processed, 2D size:" << data2D.size()
                     << "x" << (data2D.isEmpty() ? 0 : data2D[0].size());
        }
    }
    break;

    case PortraitDimension::ThreeDimensional: // [n][m][k]
    {
        // Выбираем, какую проекцию отображать в зависимости от выбранных флажков
        if (angleChecked && azimuthChecked) {
            // Выбираем средний дальностный слой
            int midRange = data.data3D[0][0].size() / 2;
            for (int i = 0; i < data.data3D.size(); ++i) {
                QVector<double> row;
                for (int j = 0; j < data.data3D[i].size(); ++j) {
                    if (data.data3D[i][j].size() > midRange) {
                        row.append(data.data3D[i][j][midRange]);
                    }
                }
                if (!row.isEmpty()) {
                    data2D.append(row);
                }
            }
            qDebug() << "ThreeDimensional (elevation-azimuth slice) data processed, 2D size:" << data2D.size()
                     << "x" << (data2D.isEmpty() ? 0 : data2D[0].size());
        } else if (angleChecked && rangeChecked) {
            // Выбираем средний азимутальный слой
            int midAzimuth = data.data3D[0].size() / 2;
            for (int i = 0; i < data.data3D.size(); ++i) {
                if (data.data3D[i].size() > midAzimuth) {
                    data2D.append(data.data3D[i][midAzimuth]);
                }
            }
            qDebug() << "ThreeDimensional (elevation-range slice) data processed, 2D size:" << data2D.size()
                     << "x" << (data2D.isEmpty() ? 0 : data2D[0].size());
        } else if (azimuthChecked && rangeChecked) {
            // Выбираем средний угломестный слой
            int midElevation = data.data3D.size() / 2;
            if (data.data3D.size() > midElevation) {
                for (int j = 0; j < data.data3D[midElevation].size(); ++j) {
                    data2D.append(data.data3D[midElevation][j]);
                }
                qDebug() << "ThreeDimensional (azimuth-range slice) data processed, 2D size:" << data2D.size()
                         << "x" << (data2D.isEmpty() ? 0 : data2D[0].size());
            }
        }
    }
    break;

    default:
        qDebug() << "Unknown dimension type:" << static_cast<int>(data.dimension);
        // Пытаемся извлечь хоть какие-то данные
        if (!data.data3D.isEmpty() && !data.data3D[0].isEmpty()) {
            if (!data.data3D[0][0].isEmpty()) {
                data1D = data.data3D[0][0];
                data2D.append(data1D);
                qDebug() << "Default handling: extracted 1D data of size:" << data1D.size();
            }
        }
        break;
    }

    // Если есть 2D данные, но нет 1D, заполняем 1D первой строкой 2D
    if (data1D.isEmpty() && !data2D.isEmpty() && !data2D[0].isEmpty()) {
        data1D = data2D[0];
        qDebug() << "Filled 1D data from first row of 2D, size:" << data1D.size();
    }

    qDebug() << "After prepare2DDataForDisplay: data1D size:" << data1D.size()
             << "data2D size:" << data2D.size()
             << (data2D.isEmpty() ? 0 : data2D[0].size());
}

void MainWindow::extract3DData(const QJsonArray &array, QVector<QVector<QVector<double>>> &container) {
    container.clear();

    qDebug() << "Размерность массива для extract3DData:" << array.size();

    for (int i = 0; i < array.size(); ++i) {
        QJsonArray dim1Array = array.at(i).toArray();
        QVector<QVector<double>> dim1Vector;

        for (int j = 0; j < dim1Array.size(); ++j) {
            QJsonArray dim2Array = dim1Array.at(j).toArray();
            QVector<double> dim2Vector;

            for (int k = 0; k < dim2Array.size(); ++k) {
                if (dim2Array.at(k).isArray()) {
                    // Если элемент - массив, извлекаем его первый элемент
                    QJsonArray innerArray = dim2Array.at(k).toArray();
                    if (!innerArray.isEmpty()) {
                        dim2Vector.append(innerArray.at(0).toDouble());
                    }
                } else {
                    // Если элемент - число, добавляем его напрямую
                    dim2Vector.append(dim2Array.at(k).toDouble());
                }
            }

            if (!dim2Vector.isEmpty()) {
                dim1Vector.append(dim2Vector);
            }
        }

        if (!dim1Vector.isEmpty()) {
            container.append(dim1Vector);
        }
    }

    // Выводим отладочную информацию о размерности полученных данных
    if (!container.isEmpty()) {
        int dim1 = container.size();
        int dim2 = !container[0].isEmpty() ? container[0].size() : 0;
        int dim3 = (dim2 > 0 && !container[0][0].isEmpty()) ? container[0][0].size() : 0;
        qDebug() << "Размерность извлеченных данных: [" << dim1 << "][" << dim2 << "][" << dim3 << "]";
    } else {
        qDebug() << "Извлеченные данные пусты!";
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

    // Определяем тип портрета
    PortraitDimension dimension = detectPortraitDimension(absEoutArray);
    qDebug() << "Detected portrait dimension: " << static_cast<int>(dimension);

    // Локальные объекты для хранения данных
    PortraitData localPortraitData;
    localPortraitData.dimension = dimension;

    // Локальные копии векторов для обратной совместимости
    QVector<double> localAbsEout;
    QVector<double> localNormEout;
    QVector<QVector<double>> localAbsEout2D;
    QVector<QVector<double>> localNormEout2D;

    // Извлекаем данные в зависимости от типа портрета
    extract3DData(absEoutArray, localPortraitData.data3D);

    // Определяем, какие чекбоксы должны быть активны в зависимости от типа данных
    bool useAngle = false, useAzimuth = false, useRange = false;

    switch (dimension) {
    case PortraitDimension::Range:
        useRange = true;
        break;
    case PortraitDimension::Azimuth:
        useAzimuth = true;
        break;
    case PortraitDimension::Elevation:
        useAngle = true;
        break;
    case PortraitDimension::AzimuthRange:
        useAzimuth = true;
        useRange = true;
        break;
    case PortraitDimension::ElevationRange:
        useAngle = true;
        useRange = true;
        break;
    case PortraitDimension::ElevationAzimuth:
        useAngle = true;
        useAzimuth = true;
        break;
    case PortraitDimension::ThreeDimensional:
        useAngle = true;
        useAzimuth = true;
        useRange = true;
        break;
    default:
        // Используем переданные параметры
        useAngle = angleChecked;
        useAzimuth = azimuthChecked;
        useRange = rangeChecked;
        break;
    }

    // Подготовка 2D данных для отображения
    prepare2DDataForDisplay(localPortraitData, useAngle, useAzimuth, useRange,
                            localAbsEout, localAbsEout2D);

    // Делаем то же самое для normEout, если нужно
    PortraitData normalizedData;
    normalizedData.dimension = dimension;
    extract3DData(normEoutArray, normalizedData.data3D);
    prepare2DDataForDisplay(normalizedData, useAngle, useAzimuth, useRange,
                            localNormEout, localNormEout2D);

    // Дополнительная отладочная информация
    qDebug() << "After processing:";
    qDebug() << "localAbsEout size:" << localAbsEout.size();
    qDebug() << "localAbsEout2D size:" << localAbsEout2D.size()
             << (localAbsEout2D.isEmpty() ? 0 : localAbsEout2D[0].size());

    // После обработки данных обновляем UI в главном потоке
    QMetaObject::invokeMethod(this, [=]() {
            portraitData = localPortraitData;
            storedResults = localStoredResults;
            absEout = localAbsEout;
            normEout = localNormEout;
            absEout2D = localAbsEout2D;
            normEout2D = localNormEout2D;

            // Устанавливаем состояние чекбоксов в соответствии с типом данных
            anglePortraitCheckBox->setChecked(useAngle);
            azimuthPortraitCheckBox->setChecked(useAzimuth);
            rangePortraitCheckBox->setChecked(useRange);

            logMessage("Числовые значения с сервера получены и сохранены.");
            logMessage(QString("Размер одномерных данных: %1, размер двумерных данных: %2x%3")
                           .arg(absEout.size())
                           .arg(absEout2D.size())
                           .arg(absEout2D.isEmpty() ? 0 : absEout2D[0].size()));

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

void MainWindow::performCalculation() {
    // Проверяем, есть ли модель
    if (!m_modelController->hasModel()) {
        logMessage("Ошибка: загруженный файл не содержит корректных данных объекта.");
        showNotification("Нет данных для расчёта", Notification::Error);
        return;
    }

    if (!anglePortraitCheckBox->isChecked() && !azimuthPortraitCheckBox->isChecked() && !rangePortraitCheckBox->isChecked()) {
        logMessage("Ошибка: тип радиопортрета не задан. Пожалуйста, выберите хотя бы один тип радиопортрета.");
        showNotification("Тип радиопортрета не задан", Notification::Error);
        return;
    }

    // Проверка наличия ConnectionManager
    if (!m_connectionManager) {
        logMessage("Ошибка: отсутствует менеджер соединений.");
        showNotification("Внутренняя ошибка приложения", Notification::Error);
        return;
    }

    // Если не подключены к серверу, предлагаем подключиться
    if (!m_connectionManager->isConnected()) {
        logMessage("Для расчёта необходимо подключение к серверу.");
        showNotification("Подключитесь к серверу", Notification::Warning);
        return;
    }

    // Сбор всех необходимых данных для расчёта
    int polarRadiation = radiationPolarizationComboBox->currentIndex();
    int polarRecive = receivePolarizationComboBox->currentIndex();
    bool typeAngle = anglePortraitCheckBox->isChecked();
    bool typeAzimut = azimuthPortraitCheckBox->isChecked();
    bool typeLength = rangePortraitCheckBox->isChecked();
    freqBand = freqBandComboBox->currentIndex();
    bool pplane = pplaneCheckBox->isChecked();

    // Получение треугольников от ModelController
    QVector<QSharedPointer<triangle>> triangles = m_modelController->getTriangles();

    // Подготовка данных о треугольниках
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

    // Получение вектора направления от ModelController
    rVect directVector = m_modelController->getDirectionVector();

    // Формирование JSON-объекта для отправки
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

    // Создание JSON-объекта для вектора направления
    QJsonObject directVectorObj;
    directVectorObj["x"] = directVector.getX();
    directVectorObj["y"] = directVector.getY();
    directVectorObj["z"] = directVector.getZ();
    modelData["directVector"] = directVectorObj;

    // Установка параметров и отправка данных
    m_connectionManager->setPolarizationAndType(polarRadiation, polarRecive, typeAngle, typeAzimut, typeLength);
    m_connectionManager->setDirectVector(directVector);

    // Проверка авторизации и отправка данных
    if (!m_connectionManager->isAuthorized()) {
        // Показываем диалог авторизации
        LoginDialog dialog(this);
        if (dialog.exec() == QDialog::Accepted) {
            QString username = dialog.getUsername();
            QString password = dialog.getPassword();
            m_connectionManager->authorize(username, password);

            // Ждем некоторое время и проверяем статус авторизации
            QTimer::singleShot(2000, this, [this, modelData]() {
                if (m_connectionManager->isAuthorized()) {
                    m_connectionManager->sendModelData(modelData);
                    abortCalculationButton->setEnabled(true);
                    setModified(true);
                } else {
                    logMessage("Ошибка авторизации. Расчёт не запущен.");
                }
            });
        }
    } else {
        // Если уже авторизованы, сразу отправляем данные
        m_connectionManager->sendModelData(modelData);
        abortCalculationButton->setEnabled(true);
        setModified(true);
    }
}

rVect MainWindow::calculateDirectVectorFromRotation() {
    QMatrix4x4 rotationMatrix;
    rotationMatrix.setToIdentity();

    // Исправление: используем поля класса с правильными именами
    rotationMatrix.rotate(inputRotationX->value(), 1.0f, 0.0f, 0.0f); // Поворот вокруг X
    rotationMatrix.rotate(inputRotationY->value(), 0.0f, 1.0f, 0.0f); // Поворот вокруг Y
    rotationMatrix.rotate(inputRotationZ->value(), 0.0f, 0.0f, 1.0f); // Поворот вокруг Z

    // Направление волны по умолчанию - в обратную сторону оси Z (новая система координат)
    QVector3D defaultDirection(0.0f, 0.0f, -1.0f);
    QVector3D transformedDirection = rotationMatrix.map(defaultDirection);

    // Преобразуем в rVect и нормализуем
    rVect directVector(transformedDirection.x(), transformedDirection.y(), transformedDirection.z());
    directVector.normalize();

    return directVector;
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
    if (absEout.isEmpty() || normEout.isEmpty()) {
        logMessage("Нет данных для графика или они не были корректно обработаны.");
        qDebug() << "Cannot open graph window: absEout size =" << absEout.size()
                 << ", normEout size =" << normEout.size();
        return;
    }

    GraphWindow *graphWindow = new GraphWindow(this);

    // Передаем информацию о частотном диапазоне и типе портрета
    graphWindow->setFreqBand(freqBand);
    graphWindow->setPortraitType(portraitData.dimension);

    // Передаем информацию о выбранных типах портрета
    bool isAngleSelected = anglePortraitCheckBox->isChecked();
    bool isAzimuthSelected = azimuthPortraitCheckBox->isChecked();
    bool isRangeSelected = rangePortraitCheckBox->isChecked();
    graphWindow->setPortraitSelections(isAngleSelected, isAzimuthSelected, isRangeSelected);

    // Если портрет многомерный (2D или 3D), передаем двумерные данные
    if (portraitData.dimension >= PortraitDimension::AzimuthRange && !absEout2D.isEmpty() && !normEout2D.isEmpty()) {
        graphWindow->setData2D(absEout2D, normEout2D);

        // Устанавливаем информационный заголовок в зависимости от типа портрета
        QString infoText;
        switch (portraitData.dimension) {
        case PortraitDimension::AzimuthRange:
            infoText = tr("Азимутально-дальностная плоскость");
            break;
        case PortraitDimension::ElevationRange:
            infoText = tr("Угломестно-дальностная плоскость");
            break;
        case PortraitDimension::ElevationAzimuth:
            infoText = tr("Угломестно-азимутальная плоскость");
            break;
        case PortraitDimension::ThreeDimensional:
            infoText = tr("Трехмерный портрет");
            break;
        default:
            break;
        }

        if (!infoText.isEmpty()) {
            graphWindow->setInfoText(infoText);
        }
    }
    else {
        // Определение шага по оси X в зависимости от freqBand и типа портрета
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

        // Создаем данные для оси X
        QVector<double> x;
        for (int i = 0; i < absEout.size(); ++i) {
            // Тип шкалы зависит от типа портрета
            if (portraitData.dimension == PortraitDimension::Range) {
                // Для дальностного портрета - шаг в метрах
                x.append(i * stepX);
            }
            else if (portraitData.dimension == PortraitDimension::Azimuth ||
                     portraitData.dimension == PortraitDimension::Elevation) {
                // Для азимутального и угломестного портретов - шаг в градусах
                // Преобразуем индекс в угол (примерное преобразование)
                double angle = i * 360.0 / absEout.size() - 180.0;
                x.append(angle);
            }
            else {
                // Для остальных типов просто используем индекс
                x.append(i);
            }
        }

        qDebug() << "Graph data prepared:"
                 << "x size:" << x.size()
                 << "absEout size:" << absEout.size()
                 << "normEout size:" << normEout.size();

        if (x.isEmpty()) {
            logMessage("Ошибка: невозможно создать ось X для графика.");
            return;
        }

        graphWindow->setData(x, absEout, normEout);
    }

    graphWindow->setAttribute(Qt::WA_DeleteOnClose);
    graphWindow->show();

    logMessage(QString("Отображен график с %1 точками данных.").arg(absEout.size()));
}

void MainWindow::showPortrait() {
    if (!showPortraitAction->isEnabled()) {
        logMessage("Двумерный портрет недоступен для текущего типа данных.");
        return;
    }

    // Проверяем наличие данных
    if (absEout2D.isEmpty()) {
        logMessage("Нет данных для отображения двумерного портрета.");
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
        // Сбор данных из OpenGLWidget и MainWindow через ModelController
        currentProjectData.triangles = m_modelController->getTriangles();
        currentProjectData.objectPosition = openGLWidget->getObjectPosition();

        // Получаем текущие углы поворота
        float rotX, rotY, rotZ;
        m_modelController->getRotation(rotX, rotY, rotZ);
        currentProjectData.rotationX = rotX;
        currentProjectData.rotationY = rotY;
        currentProjectData.rotationZ = rotZ;

        currentProjectData.scalingCoefficients = QVector3D(1.0f, 1.0f, 1.0f);
        currentProjectData.absEout = absEout;
        currentProjectData.normEout = normEout;
        currentProjectData.absEout2D = absEout2D;
        currentProjectData.normEout2D = normEout2D;
        currentProjectData.showUnderlyingSurface = pplaneCheckBox->isChecked();
        currentProjectData.surfaceAlphaValue = openGLWidget->getSurfaceAlpha();

        // Сохранение проекта
        if (ProjectSerializer::saveProject(fileName, currentProjectData)) {
            logMessage("Проект успешно сохранен: " + fileName);
            showNotification("Проект сохранен", Notification::Success);

            statusBar()->showMessage(QString("Проект: %1 | Рабочая директория: %2")
                                         .arg(currentProjectData.projectName.isEmpty() ? "Без названия" : currentProjectData.projectName)
                                         .arg(currentProjectData.workingDirectory.isEmpty() ? "Не задано" : currentProjectData.workingDirectory));

            setModified(false); // Сброс флага после успешного сохранения
            return true;
        } else {
            logMessage("Ошибка при сохранении проекта.");
            QMessageBox::critical(this, "Ошибка", "Не удалось сохранить проект.");
            showNotification("Ошибка сохранения", Notification::Error);
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

    logMessage(QString("Сетка %1").arg(visible ? "включена" : "выключена"));
}

void MainWindow::openProject() {
    if (!maybeSave()) // Проверка на несохраненные изменения перед открытием нового проекта
        return;

    QString fileName = QFileDialog::getOpenFileName(this, "Открыть проект", "", "Файлы проекта (*.projjson);;Все файлы (*.*)");
    if (!fileName.isEmpty()) {
        ProjectData data;
        if (ProjectSerializer::loadProject(fileName, data)) {
            // Установка треугольников в ModelController
            QMetaObject::invokeMethod(m_modelController.get(), [=]() {
                    m_modelController->closeModel(); // Сначала закрываем текущую модель

                    // Установка треугольников в OpenGLWidget
                    openGLWidget->setTriangles(data.triangles);

                    // Установка позиции объекта
                    openGLWidget->setObjectPosition(data.objectPosition);

                    // Установка поворотов через ModelController
                    m_modelController->setRotation(data.rotationX, data.rotationY, data.rotationZ);

                    // Восстанавливаем состояние подстилающей поверхности
                    pplaneCheckBox->setChecked(data.showUnderlyingSurface);
                    openGLWidget->setUnderlyingSurfaceVisible(data.showUnderlyingSurface);
                    openGLWidget->setSurfaceAlpha(data.surfaceAlphaValue);

                    // Установка коэффициентов масштабирования
                    openGLWidget->setScalingCoefficients(data.scalingCoefficients);
                }, Qt::QueuedConnection);

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

// Метод для создания нового проекта
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

    // Очистка модели через ModelController
    m_modelController->closeModel();

    // Очистка результатов
    absEout.clear();
    normEout.clear();
    absEout2D.clear();
    normEout2D.clear();
    storedResults.clear();

    // Сброс состояния чекбоксов и других элементов UI
    anglePortraitCheckBox->setChecked(false);
    azimuthPortraitCheckBox->setChecked(false);
    rangePortraitCheckBox->setChecked(false);
    pplaneCheckBox->setChecked(false);
    gridCheckBox->setChecked(false);
    freqBandComboBox->setCurrentIndex(5);
    radiationPolarizationComboBox->setCurrentIndex(0);
    receivePolarizationComboBox->setCurrentIndex(0);

    // Вывод сообщения в журнал
    logMessage("Создан новый проект.");

    setModified(false); // Сброс флага после создания нового проекта
}

void MainWindow::updateMenuActions() {
    // Сначала проверяем, есть ли у нас данные вообще
    if (portraitData.data3D.isEmpty()) {
        hasNumericalData = false;
        hasGraphData = false;
        openResultsAction->setEnabled(false);
        openGraphAction->setEnabled(false);
        showPortraitAction->setEnabled(false);
        return;
    }

    // Активируем кнопки на основе типа данных
    switch (portraitData.dimension) {
    case PortraitDimension::Range:
    case PortraitDimension::Azimuth:
    case PortraitDimension::Elevation:
        // Для одномерных портретов активируем числовые значения и одномерный график
        if (!absEout.isEmpty()) {
            hasNumericalData = true;
            hasGraphData = true;
            openResultsAction->setEnabled(true);
            openGraphAction->setEnabled(true);
        } else {
            hasNumericalData = false;
            hasGraphData = false;
            openResultsAction->setEnabled(false);
            openGraphAction->setEnabled(false);
        }
        showPortraitAction->setEnabled(false);
        break;

    case PortraitDimension::ElevationRange:
    case PortraitDimension::AzimuthRange:
    case PortraitDimension::ElevationAzimuth:
    case PortraitDimension::ThreeDimensional:
        // Для двумерных и трехмерных портретов
        hasNumericalData = true;
        openResultsAction->setEnabled(true);

        // График активен только если есть данные
        if (!absEout.isEmpty()) {
            hasGraphData = true;
            openGraphAction->setEnabled(true);
        } else {
            hasGraphData = false;
            openGraphAction->setEnabled(false);
        }

        // Двумерный портрет активен только если есть двумерные данные
        showPortraitAction->setEnabled(!absEout2D.isEmpty());
        break;

    default:
        // Неизвестный тип - ничего не активируем
        hasNumericalData = false;
        hasGraphData = false;
        openResultsAction->setEnabled(false);
        openGraphAction->setEnabled(false);
        showPortraitAction->setEnabled(false);
        break;
    }

    // Вывод информации о типе портрета в лог для отладки
    QString dimensionName;
    switch (portraitData.dimension) {
    case PortraitDimension::Range: dimensionName = "Дальностный"; break;
    case PortraitDimension::Azimuth: dimensionName = "Азимутальный"; break;
    case PortraitDimension::Elevation: dimensionName = "Угломестный"; break;
    case PortraitDimension::AzimuthRange: dimensionName = "Азимутальный+Дальностный"; break;
    case PortraitDimension::ElevationRange: dimensionName = "Угломестный+Дальностный"; break;
    case PortraitDimension::ElevationAzimuth: dimensionName = "Угломестный+Азимутальный"; break;
    case PortraitDimension::ThreeDimensional: dimensionName = "Трехмерный"; break;
    default: dimensionName = "Неизвестный"; break;
    }

    qDebug() << "Тип портрета:" << dimensionName
             << "1D данные (размер):" << absEout.size()
             << "2D данные (размер):" << absEout2D.size() << "x" << (absEout2D.isEmpty() ? 0 : absEout2D[0].size())
             << "Числовые значения:" << hasNumericalData
             << "Одномерный график:" << hasGraphData
             << "Двумерный портрет:" << showPortraitAction->isEnabled();

    // Добавим сообщение в лог для пользователя
    logMessage(QString("Данные портрета типа '%1' готовы к просмотру.").arg(dimensionName));
}

void MainWindow::onPortraitTypeChanged() {
    // Проверяем, какие типы портретов выбраны
    bool isAngleChecked = anglePortraitCheckBox->isChecked();
    bool isAzimuthChecked = azimuthPortraitCheckBox->isChecked();
    bool isRangeChecked = rangePortraitCheckBox->isChecked();

    // Если есть данные портрета, обновляем отображение
    if (!portraitData.data3D.isEmpty()) {
        QVector<double> tempAbsEout;
        QVector<QVector<double>> tempAbsEout2D;
        QVector<double> tempNormEout;
        QVector<QVector<double>> tempNormEout2D;

        // Готовим данные для отображения
        prepare2DDataForDisplay(portraitData, isAngleChecked, isAzimuthChecked, isRangeChecked,
                                tempAbsEout, tempAbsEout2D);

        // Обновляем данные класса
        absEout = tempAbsEout;
        absEout2D = tempAbsEout2D;

        // Подготавливаем данные для нормализованного отображения
        PortraitData normalizedData;
        normalizedData.dimension = portraitData.dimension;
        normalizedData.data3D = portraitData.data3D; // Используем те же данные для простоты

        prepare2DDataForDisplay(normalizedData, isAngleChecked, isAzimuthChecked, isRangeChecked,
                                tempNormEout, tempNormEout2D);

        normEout = tempNormEout;
        normEout2D = tempNormEout2D;

        // Логируем изменения выбранных типов портретов
        QStringList selected;
        if (isAngleChecked) selected << "Угломестный";
        if (isAzimuthChecked) selected << "Азимутальный";
        if (isRangeChecked) selected << "Дальностный";

        logMessage(QString("Выбраны типы портретов: %1").arg(selected.join(", ")));
    }

    // Больше не вызываем updateMenuActions() здесь
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

void MainWindow::updateFilterStats(const MeshFilter::FilterStats& stats) {
    shellCountDisplay->setText(QString::number(stats.shellTriangles));
    visibleCountDisplay->setText(QString::number(stats.visibleTriangles));
    removedShellDisplay->setText(QString::number(stats.removedByShell));
    removedShadowDisplay->setText(QString::number(stats.removedByShadow));
}

void MainWindow::showNotification(const QString &message, Notification::Type type) {
    NotificationManager::instance()->showMessage(message, type);
}

void MainWindow::abortCalculation() {
    if (!m_connectionManager) {
        logMessage("Ошибка: отсутствует менеджер соединений.");
        return;
    }

    if (!m_connectionManager->isConnected()) {
        logMessage("Нет активного подключения к серверу");
        return;
    }

    m_connectionManager->abortCalculation();
    // UI элементы обновляются в методе onCalculationAborted(),
    // который вызывается через механизм наблюдателя
}

void MainWindow::updateCalculationProgress(int progress) {
    if (progressBar) {
        progressBar->setValue(progress);

        // // Изменяем цвет прогресс-бара в зависимости от прогресса
        // QString styleSheet;
        // if (progress < 30) {
        //     styleSheet = "QProgressBar::chunk { background-color: #FF6B6B; }";
        // } else if (progress < 70) {
        //     styleSheet = "QProgressBar::chunk { background-color: #FFD93D; }";
        // } else {
        //     styleSheet = "QProgressBar::chunk { background-color: #6BCB77; }";
        // }
        // progressBar->setStyleSheet(styleSheet);
    }
}

void MainWindow::showPatternDiagram() {
    if (!patternDiagramWindow) {
        patternDiagramWindow = new PatternDiagramWindow(this);
    }

    // Если у нас есть данные, отобразим их
    if (!absEout2D.isEmpty()) {
        patternDiagramWindow->setData(absEout2D);
        patternDiagramWindow->show();
    } else {
        // Если нет данных, можно показать тестовые данные или вывести сообщение
        QMessageBox::warning(this, "Ошибка", "Нет данных для отображения диаграммы направленности. "
                                             "Сначала выполните расчёт или загрузите результаты.");
    }
}

void MainWindow::onConnectionStatusChanged(bool connected)
{
    // Логика обработки изменения статуса подключения
    emit connectionStatusChanged(connected);
}

void MainWindow::onResultsReceived(const QJsonObject &results)
{
    displayResults(results);
    showNotification("Получены новые результаты", Notification::Success);
    abortCalculationButton->setEnabled(false);
    setModified(true);
}

void MainWindow::onCalculationProgress(int progress)
{
    updateCalculationProgress(progress);
}

void MainWindow::onLogMessage(const QString &message)
{
    logMessage(message);
}

void MainWindow::onNotification(const QString &message, Notification::Type type)
{
    showNotification(message, type);
}

void MainWindow::onCalculationAborted()
{
    abortCalculationButton->setEnabled(false);
    progressBar->setValue(0);
}

// Реализация методов интерфейса IModelObserver

void MainWindow::onModelLoaded(const QString &fileName, int nodesCount, int trianglesCount) {
    QFileInfo fileInfo(fileName);
    modelFileNameLabel->setText(fileInfo.fileName());
    totalNodesLabel->setText(QString("%1").arg(nodesCount));
    totalTrianglesLabel->setText(QString("%1").arg(trianglesCount));

    // Обновляем UI-элементы, которые зависят от наличия загруженной модели
    // Например, активировать кнопку закрытия модели
    QList<QAction*> actions = menuBar()->actions();
    for (QAction* action : actions) {
        if (action->text() == "Файл") {
            QMenu* fileMenu = action->menu();
            QList<QAction*> fileActions = fileMenu->actions();
            for (QAction* fileAction : fileActions) {
                if (fileAction->text() == "Закрыть") {
                    fileAction->setEnabled(true);
                    break;
                }
            }
            break;
        }
    }

    setModified(true);
}

void MainWindow::onModelModified() {
    setModified(true);
}

void MainWindow::onModelClosed() {
    modelFileNameLabel->setText("не загружен");
    totalNodesLabel->setText("0");
    totalTrianglesLabel->setText("0");

    // Деактивировать кнопку закрытия модели
    QList<QAction*> actions = menuBar()->actions();
    for (QAction* action : actions) {
        if (action->text() == "Файл") {
            QMenu* fileMenu = action->menu();
            QList<QAction*> fileActions = fileMenu->actions();
            for (QAction* fileAction : fileActions) {
                if (fileAction->text() == "Закрыть") {
                    fileAction->setEnabled(false);
                    break;
                }
            }
            break;
        }
    }

    setModified(true);
}

void MainWindow::onFilteringCompleted(const MeshFilter::FilterStats &stats) {
    // Всегда обновляем общее количество треугольников
    totalTrianglesLabel->setText(QString::number(stats.totalTriangles));

    // Проверка на случай, если модель была закрыта или очищена
    if (stats.totalTriangles == 0) {
        // Сбрасываем все значения статистики на "0"
        shellCountDisplay->setText("0");
        visibleCountDisplay->setText("0");
        removedShellDisplay->setText("0");
        removedShadowDisplay->setText("0");
        return;
    }

    // Обновляем статистику в зависимости от типа выполненной фильтрации
    switch (stats.filterType) {
    case MeshFilter::ShellFilter:
        // При фильтрации оболочки обновляем только соответствующие поля
        shellCountDisplay->setText(QString::number(stats.shellTriangles));
        removedShellDisplay->setText(QString::number(stats.removedByShell));

        // Текущие значения для видимости сохраняем
        if (stats.visibleTriangles > 0) {
            visibleCountDisplay->setText(QString::number(stats.visibleTriangles));
            removedShadowDisplay->setText(QString::number(stats.removedByShadow));
        }
        break;

    case MeshFilter::ShadowFilter:
        // При фильтрации теней обновляем только соответствующие поля
        visibleCountDisplay->setText(QString::number(stats.visibleTriangles));
        removedShadowDisplay->setText(QString::number(stats.removedByShadow));

        // Текущие значения для оболочки сохраняем
        if (stats.shellTriangles > 0) {
            shellCountDisplay->setText(QString::number(stats.shellTriangles));
            removedShellDisplay->setText(QString::number(stats.removedByShell));
        }
        break;

    case MeshFilter::ResetFilter:
        // При сбросе обновляем все поля
        shellCountDisplay->setText(QString::number(stats.shellTriangles));
        visibleCountDisplay->setText(QString::number(stats.visibleTriangles));
        removedShellDisplay->setText(QString::number(stats.removedByShell));
        removedShadowDisplay->setText(QString::number(stats.removedByShadow));
        break;

    default:
        // По умолчанию обновляем все поля
        shellCountDisplay->setText(QString::number(stats.shellTriangles));
        visibleCountDisplay->setText(QString::number(stats.visibleTriangles));
        removedShellDisplay->setText(QString::number(stats.removedByShell));
        removedShadowDisplay->setText(QString::number(stats.removedByShadow));
        break;
    }
}

// Реализация слотов для интеграции с ModelController

// Метод для
void MainWindow::handleLoadModel() {
    QString fileName = QFileDialog::getOpenFileName(this, "Открыть 3D модель", "", "OBJ Files (*.obj)");
    if (!fileName.isEmpty()) {
        progressBar->setValue(0);

        // Используем QtConcurrent::run для выполнения в отдельном потоке
        QFutureWatcher<void> *watcher = new QFutureWatcher<void>(this);
        connect(watcher, &QFutureWatcher<void>::finished, this, [=]() {
            watcher->deleteLater();
        });

        QFuture<void> future = QtConcurrent::run([=]() {
            // Вызываем метод ModelController
            QMetaObject::invokeMethod(m_modelController.get(), [=]() {
                    m_modelController->loadModel(fileName);
                }, Qt::QueuedConnection);
        });

        watcher->setFuture(future);
    } else {
        logMessage("Ошибка: файл не был выбран.");
    }
}

void MainWindow::handleApplyRotation() {
    float rotationX = inputRotationX->value();
    float rotationY = inputRotationY->value();
    float rotationZ = inputRotationZ->value();

    m_modelController->setRotation(rotationX, rotationY, rotationZ);
}

void MainWindow::handleResetRotation() {
    m_modelController->resetRotation();
}

void MainWindow::handleFilterModel() {
    m_modelController->filterMesh();
}

void MainWindow::handleToggleShadowTriangles() {
    m_modelController->toggleShadowTriangles();
}

void MainWindow::handleCloseModel() {
    m_modelController->closeModel();
}

void MainWindow::handleResetScale() {
    m_modelController->resetScale();

    // Выводим сообщение в журнал
    logMessage("Масштаб объекта сброшен к исходному значению (1.0)");
}
