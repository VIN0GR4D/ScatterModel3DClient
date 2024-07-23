#ifndef CALCTOOLS_H
#define CALCTOOLS_H
#include <QDateTime>
#include <QFile>
#include <QString>
#include <QTextStream>
#include <QWebSocket>
#include <QJsonDocument>
#include <QJsonObject>

void clogs(QString msg, QString type, QString Source);
void webServerAnswer(QString answer, QWebSocket *pClient);
extern QScopedPointer<QFile> m_logFile;

#endif // CALCTOOLS_H
