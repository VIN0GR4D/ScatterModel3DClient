#include "resultsdialog.h"
#include <QVBoxLayout>
#include <QDebug>

ResultsDialog::ResultsDialog(QWidget *parent) : QDialog(parent) {
    qDebug() << "ResultsDialog constructor started"; // Логирование начала конструктора

    QTextEdit *resultsTextEdit = new QTextEdit(this); // Локальная переменная
    resultsTextEdit->setReadOnly(true);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(resultsTextEdit);
    setLayout(layout);

    setWindowTitle("Results");
    this->resize(800, 600);

    qDebug() << "ResultsDialog constructor finished"; // Логирование окончания конструктора
}

void ResultsDialog::setResults(const QString &results) {
    QTextEdit *resultsTextEdit = findChild<QTextEdit *>();
    if (resultsTextEdit) {
        resultsTextEdit->setText(results);
    }
}
