#ifndef CLIENTAI_H
#define CLIENTAI_H
#include <QObject>
#include <QDateTime>
#include <QWebSocket>


class clientAI: public QObject
{
    Q_OBJECT

public:
    clientAI(QObject *parent = nullptr);
    QString id;
    QDate dateConnect;
    QTime timeConnect;
    QWebSocket *pClient;
    bool authStatus = false;
    QString login;
    QString email;

public slots:
    void auth();
};
Q_DECLARE_METATYPE(clientAI*);
#endif // CLIENTAI_H
