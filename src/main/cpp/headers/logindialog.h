#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    LoginDialog(QWidget *parent = nullptr) : QDialog(parent) {
        QVBoxLayout *layout = new QVBoxLayout(this);

        QLabel *labelUsername = new QLabel("Username:", this);
        editUsername = new QLineEdit(this);

        QLabel *labelPassword = new QLabel("Password:", this);
        editPassword = new QLineEdit(this);
        editPassword->setEchoMode(QLineEdit::Password);

        QPushButton *buttonLogin = new QPushButton("Login", this);
        connect(buttonLogin, &QPushButton::clicked, this, &LoginDialog::accept);

        layout->addWidget(labelUsername);
        layout->addWidget(editUsername);
        layout->addWidget(labelPassword);
        layout->addWidget(editPassword);
        layout->addWidget(buttonLogin);

        setLayout(layout);
    }

    QString getUsername() const {
        return editUsername->text();
    }

    QString getPassword() const {
        return editPassword->text();
    }

private:
    QLineEdit *editUsername;
    QLineEdit *editPassword;
};
