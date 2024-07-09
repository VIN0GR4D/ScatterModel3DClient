#ifndef RESULTSDIALOG_H
#define RESULTSDIALOG_H

#include <QDialog>
#include <QTextEdit>

class ResultsDialog : public QDialog {
    Q_OBJECT

public:
    explicit ResultsDialog(QWidget *parent = nullptr);
    void setResults(const QString &results);

// private:
//     QTextEdit *resultsTextEdit;
};

#endif // RESULTSDIALOG_H
