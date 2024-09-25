#include "aboutdialog.h"

AboutDialog::AboutDialog(QWidget *parent) : QDialog(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *titleLabel = new QLabel("<h1 style='color:#007ACC;'>ScatterModel3DClient</h1>", this);
    QLabel *versionLabel = new QLabel("<p><strong>Версия:</strong> 1.0.0</p>", this);
    QLabel *descriptionLabel = new QLabel("<p>Это приложение предназначено для анализа рассеяния электромагнитных волн на 3D объекты.</p>", this);
    QLabel *authorLabel = new QLabel("<p><strong>Разработчик:</strong> АО НЦПЭ 2024</p>", this);
    QLabel *authorInfoLabel = new QLabel("<p><strong>Автор:</strong> VINOGRAD</p>", this);

    QLabel *guideLabel = new QLabel(
        "<h2 style='color:#007ACC;'>Гайд по использованию:</h2>"
        "<ol>"
        "<li>Подключитесь к серверу, используя панель управления.</li>"
        "<li>Загрузите объект с помощью \"Файл\" → \"Открыть\".</li>"
        "<li>Укажите необходимый(-ые) тип портрета, параметры и угол поворота объекта.</li>"
        "<li>Запустите процесс расчёта с помощью \"Выполнить\" → \"Выполнить расчёт\".</li>"
        "<li>Авторизируйтесь во всплывающем окне.</li>"
        "<li>Во вкладке \"Результаты\" выберите необходимую визуализацию расчётов.</li>"
        "</ol>", this);

    QLabel *rightsLabel = new QLabel("<p style='text-align:center;'><strong>Все права защищены ©️</strong></p>", this);

    QPushButton *okButton = new QPushButton("OK", this);
    connect(okButton, &QPushButton::clicked, this, &AboutDialog::accept);

    layout->addWidget(titleLabel);
    layout->addWidget(versionLabel);
    layout->addWidget(descriptionLabel);
    layout->addWidget(authorLabel);
    layout->addWidget(authorInfoLabel);
    layout->addSpacing(20);  // Отступ перед гайдом
    layout->addWidget(guideLabel);
    layout->addSpacing(20);  // Отступ перед кнопкой
    layout->addWidget(okButton, 0, Qt::AlignCenter);
    okButton->setFixedSize(200, 30);
    layout->addSpacing(10);  // Отступ перед информацией об авторских правах
    layout->addWidget(rightsLabel, 0, Qt::AlignCenter);  // Авторские права по центру

    setLayout(layout);
    setWindowTitle("О программе");
    setFixedSize(600, 400);
}
