#include "newprojectdialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QMessageBox>

NewProjectDialog::NewProjectDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle("Новый проект");
    setModal(true);

    QLabel *nameLabel = new QLabel("Название проекта:", this);
    projectNameEdit = new QLineEdit(this);

    QLabel *dirLabel = new QLabel("Рабочая директория:", this);
    workingDirEdit = new QLineEdit(this);
    workingDirEdit->setReadOnly(true);
    browseButton = new QPushButton("Обзор...", this);

    // Соединение кнопки "Обзор..." с слотом
    connect(browseButton, &QPushButton::clicked, this, &NewProjectDialog::browseDirectory);

    // Кнопки OK и Cancel
    okButton = new QPushButton("OK", this);
    cancelButton = new QPushButton("Отмена", this);

    connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // Компоновка для выбора рабочей директории
    QHBoxLayout *dirLayout = new QHBoxLayout();
    dirLayout->addWidget(workingDirEdit);
    dirLayout->addWidget(browseButton);

    // Компоновка для кнопок OK и Cancel
    QHBoxLayout *buttonsLayout = new QHBoxLayout();
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(okButton);
    buttonsLayout->addWidget(cancelButton);

    // Основная компоновка
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(nameLabel);
    mainLayout->addWidget(projectNameEdit);
    mainLayout->addWidget(dirLabel);
    mainLayout->addLayout(dirLayout);
    mainLayout->addLayout(buttonsLayout);

    setLayout(mainLayout);
}

QString NewProjectDialog::getProjectName() const {
    return projectNameEdit->text().trimmed();
}

QString NewProjectDialog::getWorkingDirectory() const {
    return workingDirEdit->text().trimmed();
}

void NewProjectDialog::browseDirectory() {
    QString dir = QFileDialog::getExistingDirectory(this, "Выбрать рабочую директорию", QString(),
                                                    QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) {
        workingDirEdit->setText(dir);
    }
}
