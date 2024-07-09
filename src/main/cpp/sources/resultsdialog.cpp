#include "resultsdialog.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QVBoxLayout>

ResultsDialog::ResultsDialog(QWidget *parent) : QDialog(parent) {
    resultsTextEdit = new QTextEdit(this);
    resultsTextEdit->setReadOnly(true);

    saveResultsButton = new QPushButton("Сохранить результаты", this);
    connect(saveResultsButton, &QPushButton::clicked, this, &ResultsDialog::saveResults);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(resultsTextEdit);
    layout->addWidget(saveResultsButton);
    setLayout(layout);

    setWindowTitle("Числовые значения");

    // Задаем размер окна
    this->resize(800, 600);
}

void ResultsDialog::setResults(const QString &results) {
    resultsTextEdit->setText(results);
}

void ResultsDialog::saveResults() {
    QString fileName = QFileDialog::getSaveFileName(this, "Числовые значения", "", "Text Files (*.txt)");
    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << resultsTextEdit->toPlainText();
        }
    }
}
