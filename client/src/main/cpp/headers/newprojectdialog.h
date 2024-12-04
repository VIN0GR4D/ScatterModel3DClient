#ifndef NEWPROJECTDIALOG_H
#define NEWPROJECTDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class NewProjectDialog : public QDialog {
    Q_OBJECT

public:
    NewProjectDialog(QWidget *parent = nullptr);

    QString getProjectName() const;
    QString getWorkingDirectory() const;

private slots:
    void browseDirectory();

private:
    QLineEdit *projectNameEdit;
    QLineEdit *workingDirEdit;
    QPushButton *browseButton;
    QPushButton *okButton;
    QPushButton *cancelButton;
};

#endif // NEWPROJECTDIALOG_H
