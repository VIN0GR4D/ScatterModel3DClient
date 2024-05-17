#include "triangleclient.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QTimer>

TriangleClient::TriangleClient(const QUrl &url, QObject *parent)
    : QObject(parent), m_webSocket(new QWebSocket), m_url(url) {
    connect(m_webSocket, &QWebSocket::connected, this, &TriangleClient::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &TriangleClient::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &TriangleClient::onTextMessageReceived);
    connect(m_webSocket, &QWebSocket::errorOccurred, this, &TriangleClient::onErrorOccurred);
    m_webSocket->open(url);
}

TriangleClient::~TriangleClient() {
    m_webSocket->close();
}

void TriangleClient::onConnected() {
    qDebug() << "WebSocket connected, preparing to send data...";
    QVector<QSharedPointer<triangle>> triangles;
    if (!triangles.isEmpty()) {
        sendTriangleData(triangles);
    }
}

void TriangleClient::onDisconnected() {
    qDebug() << "WebSocket disconnected, attempting to reconnect...";
    QTimer::singleShot(5000, this, [this]() { m_webSocket->open(m_url); });
}

void TriangleClient::onErrorOccurred(QAbstractSocket::SocketError error) {
    qDebug() << "WebSocket error occurred, code:" << error;
    switch (error) {
    case QAbstractSocket::ConnectionRefusedError:
        qDebug() << "Connection Refused by the server.";
        break;
    case QAbstractSocket::RemoteHostClosedError:
        qDebug() << "Remote host closed the connection.";
        break;
    case QAbstractSocket::HostNotFoundError:
        qDebug() << "Host not found.";
        break;
    default:
        qDebug() << "Unhandled socket error.";
        break;
    }

    if (error == QAbstractSocket::ConnectionRefusedError ||
        error == QAbstractSocket::RemoteHostClosedError ||
        error == QAbstractSocket::HostNotFoundError) {
        qDebug() << "Attempting to reconnect...";
        QTimer::singleShot(5000, this, [this]() { m_webSocket->open(m_url); });
    }
}

void TriangleClient::onTextMessageReceived(QString message) {
    qDebug() << "Message received:" << message;
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject obj = doc.object();

    if (obj.contains("type")) {
        QString type = obj["type"].toString();
        if (type == "result") {
            qDebug() << "Results received from server.";
            emit resultsReceived(obj["data"].toString());
        }
    }
}

void TriangleClient::authorize(const QString &username, const QString &password) {
    QJsonObject authObject = {
        {"type", "auth"},
        {"login", username},
        {"password", password}
    };
    QJsonDocument doc(authObject);
    m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
    qDebug() << "Attempting to authorize...";
}

void TriangleClient::sendTriangleData(const QVector<QSharedPointer<triangle>>& triangles) {
    if (!m_webSocket->isValid()) {
        qDebug() << "WebSocket is not connected. Attempting to resend data...";
        QTimer::singleShot(5000, this, [this, triangles]() { sendTriangleData(triangles); });
        return;
    }

    QJsonArray triangleArray;
    for (const auto& tri : triangles) {
        if (tri->getVisible()) {
            QJsonObject triObject = {
                {"v1", vectorToJson(tri->getV1())},
                {"v2", vectorToJson(tri->getV2())},
                {"v3", vectorToJson(tri->getV3())}
            };
            triangleArray.append(triObject);
        }
    }

    QJsonObject messageObject = {
        {"type", "triangles"},
        {"data", triangleArray}
    };
    QJsonDocument doc(messageObject);
    m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
    qDebug() << "Sending triangle data to server...";
}

QJsonObject TriangleClient::vectorToJson(const QSharedPointer<const rVect>& vector) {
    return {
        {"x", vector->getX()},
        {"y", vector->getY()},
        {"z", vector->getZ()}
    };
}

void TriangleClient::sendCommand(const QString &command) {
    if (m_webSocket->isValid()) {
        QJsonObject messageObject = {
            {"type", "command"},
            {"command", command}
        };
        QJsonDocument doc(messageObject);
        QString dataString = doc.toJson(QJsonDocument::Compact);
        qDebug() << "Sending data to server:" << dataString;
        m_webSocket->sendTextMessage(dataString);
        qDebug() << "Sending" << command << "command to server...";
    }
}

void TriangleClient::processResults(const QJsonObject &results) {
    qDebug() << "Processing results from server...";
    if (results.contains("content") && results["type"].toString() == "radioportrait") {
        QJsonObject radioPortrait = results["content"].toObject();
        // Обработка данных радиопортрета
        qDebug() << "Radio Portrait Data: " << radioPortrait;
    } else if (results.contains("progress")) {
        int progress = results["progress"].toInt();
        qDebug() << "Progress: " << progress << "%";
    } else {
        qDebug() << "Unknown result type or content";
    }
}
