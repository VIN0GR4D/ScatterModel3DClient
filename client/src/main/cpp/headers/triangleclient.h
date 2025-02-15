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
#include <memory>
#include <QThread>
#include <QMutex>
#include <QQueue>
#include "Triangle.h"
#include "logworker.h"
#include "notification.h"

class TriangleClient : public QObject {
    Q_OBJECT
public:
    explicit TriangleClient(const QUrl &url, QObject *parent = nullptr);
    ~TriangleClient();

    bool isConnected() const;
    void authorize(const QString &username, const QString &password);
    void sendCommand(const QJsonObject &commandObject);
    void setPolarizationAndType(int polarRadiation, int polarRecive, bool typeAngle, bool typeAzimut, bool typeLength);
    void sendModelData(const QJsonObject &modelData);
    void setDirectVector(const rVect& directVector);

    int getPolarRadiation() const { return m_polarRadiation; }
    int getPolarRecive() const { return m_polarRecive; }
    bool getTypeAngle() const { return m_typeAngle; }
    bool getTypeAzimut() const { return m_typeAzimut; }
    bool getTypeLength() const { return m_typeLength; }

    bool isAuthorized() const;

    void disconnectFromServer();

    QWebSocket* getWebSocket() const { return m_webSocket.get(); }

signals:
    void resultsReceived(const QJsonObject &results);
    void logMessage(const QString &message);
    void logToFile(const QString &message);
    void showNotification(const QString &message, Notification::Type type);
    void calculationAborted();
    void progressUpdated(int progress);

public slots:
    void onConnected();
    void onDisconnected();
    void onTextMessageReceived(QString message);
    void parseAndProcessMessage(const QString& message);
    void sendTriangleData(const QVector<QSharedPointer<triangle>>& triangles);
    void onErrorOccurred(QAbstractSocket::SocketError error);

private:
    std::unique_ptr<QWebSocket> m_webSocket;
    QUrl m_url;
    int m_polarRadiation;
    int m_polarRecive;
    bool m_typeAngle;
    bool m_typeAzimut;
    bool m_typeLength;
    QJsonObject vectorToJson(const QSharedPointer<const rVect>& vector);
    void processResults(const QJsonObject &results);
    int m_reconnectAttempts;
    const int m_maxReconnectAttempts;
    bool m_intentionalDisconnect;
    rVect m_directVector;
    bool m_isAuthorized = false;

    void attemptReconnect();
    QThread m_logThread;
    LogWorker *m_logWorker;

    bool m_calculationAborted;  // Флаг прерванного состояния
};

#endif // TRIANGLECLIENT_H
