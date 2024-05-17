#ifndef TRIANGLECLIENT_H
#define TRIANGLECLIENT_H

#include <QObject>
#include <QWebSocket>
#include <QSharedPointer>
#include <QVector>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include "triangle.h"

class TriangleClient : public QObject {
    Q_OBJECT
public:
    explicit TriangleClient(const QUrl &url, QObject *parent = nullptr);
    ~TriangleClient();
    void sendCommand(const QString &command);

signals:
    void resultsReceived(const QString &results);

public slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(QString message);
    void sendTriangleData(const QVector<QSharedPointer<triangle>>& triangles);
    void onErrorOccurred(QAbstractSocket::SocketError error);

private:
    QWebSocket *m_webSocket;
    QUrl m_url;
    QJsonObject vectorToJson(const QSharedPointer<const rVect>& vector);
    void processResults(const QJsonObject &results);
    void authorize(const QString &username, const QString &password);
};

#endif // TRIANGLECLIENT_H
