#include "logindialog.h"

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent) {
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *labelUsername = new QLabel("Логин", this);
    editUsername = new QLineEdit(this);

    QLabel *labelPassword = new QLabel("Пароль", this);
    editPassword = new QLineEdit(this);
    editPassword->setEchoMode(QLineEdit::Password);

    QPushButton *buttonLogin = new QPushButton("Авторизоваться", this);
    buttonLogin->setIcon(QIcon(":/login-icon.png"));
    connect(buttonLogin, &QPushButton::clicked, this, &LoginDialog::accept);

    // Добавление виджетов в компоновку
    mainLayout->addWidget(labelUsername);
    mainLayout->addWidget(editUsername);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(labelPassword);
    mainLayout->addWidget(editPassword);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(buttonLogin);

    // Настройка размеров полей и кнопки
    labelUsername->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    labelPassword->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    editUsername->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    editPassword->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    buttonLogin->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    setLayout(mainLayout);
    setWindowTitle("Авторизация");

    // Задаем минимальный размер окна
    this->setMinimumSize(250, 150);
}

QString LoginDialog::getUsername() const {
    return editUsername->text();
}

QString LoginDialog::getPassword() const {
    return editPassword->text();
}
