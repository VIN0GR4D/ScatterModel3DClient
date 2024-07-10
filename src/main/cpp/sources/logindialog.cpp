#include "logindialog.h"

LoginDialog::LoginDialog(QWidget *parent) : QDialog(parent) {
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *logoLabel = new QLabel(this);
    QPixmap logoPixmap(":/icon.png");
    logoLabel->setPixmap(logoPixmap);
    logoLabel->setAlignment(Qt::AlignCenter);

    QLabel *labelUsername = new QLabel("Логин:", this);
    editUsername = new QLineEdit(this);

    QLabel *labelPassword = new QLabel("Пароль:", this);
    editPassword = new QLineEdit(this);
    editPassword->setEchoMode(QLineEdit::Password);

    QPushButton *buttonLogin = new QPushButton("Авторизоваться", this);
    buttonLogin->setIcon(QIcon(":/login-icon.png"));
    connect(buttonLogin, &QPushButton::clicked, this, &LoginDialog::accept);

    layout->addWidget(labelUsername);
    layout->addWidget(editUsername);
    layout->addWidget(labelPassword);
    layout->addWidget(editPassword);
    layout->addWidget(buttonLogin);

    setLayout(layout);
    setWindowTitle("Авторизация");

    // Задаем размер окна
    this->resize(300, 150);
}

QString LoginDialog::getUsername() const {
    return editUsername->text();
}

QString LoginDialog::getPassword() const {
    return editPassword->text();
}
