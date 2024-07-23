#ifndef RESULTSDIALOG_H
#define RESULTSDIALOG_H

#include <QDialog>
#include <QTextEdit>
#include <QPushButton>
#include <QVBoxLayout>

class ResultsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ResultsDialog(QWidget *parent = nullptr);
    void setResults(const QString &results);

private slots:
    void saveResults();

private:
    QTextEdit *resultsTextEdit;
    QPushButton *saveResultsButton;
};

#endif // RESULTSDIALOG_H
