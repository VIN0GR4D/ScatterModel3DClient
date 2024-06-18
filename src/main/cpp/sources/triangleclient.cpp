#include "triangleclient.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QTimer>
#include <QFile>

TriangleClient::TriangleClient(const QUrl &url, QObject *parent)
    : QObject(parent), m_webSocket(new QWebSocket), m_url(url), m_reconnectAttempts(0), m_maxReconnectAttempts(3) {
    connect(m_webSocket, &QWebSocket::connected, this, &TriangleClient::onConnected);
    connect(m_webSocket, &QWebSocket::disconnected, this, &TriangleClient::onDisconnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &TriangleClient::onTextMessageReceived);
    connect(m_webSocket, &QWebSocket::errorOccurred, this, &TriangleClient::onErrorOccurred);
    m_webSocket->open(url); // Открываем соединение с сервером
}

TriangleClient::~TriangleClient() {
    m_webSocket->close();
}
// Попытка переподключения
void TriangleClient::attemptReconnect() {
    if (m_reconnectAttempts < m_maxReconnectAttempts) {
        m_reconnectAttempts++;
        QTimer::singleShot(5000, this, [this]() { m_webSocket->open(m_url); });
    } else {
        emit logMessage("Достигнуто максимальное количество попыток переподключения.");
        qDebug() << "Достигнуто максимальное количество попыток переподключения.";
    }
}

// Обработчик события успешного подключения к серверу
void TriangleClient::onConnected() {
    emit logMessage("WebSocket connected");
    qDebug() << "WebSocket connected, preparing to send data...";
    m_reconnectAttempts = 0;
}

bool TriangleClient::isConnected() const {
    return m_webSocket->isValid() && m_webSocket->state() == QAbstractSocket::ConnectedState;
}

// Обработчик события отключения от сервера
void TriangleClient::onDisconnected() {
    emit logMessage("WebSocket disconnected");
    qDebug() << "WebSocket disconnected, attempting to reconnect...";
    attemptReconnect();
}

// Обработчик ошибок соединения
void TriangleClient::onErrorOccurred(QAbstractSocket::SocketError error) {
    emit logMessage("WebSocket error occurred, code: " + QString::number(error));
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
        attemptReconnect();
    }
}

// Обработчик получения текстового сообщения от сервера
void TriangleClient::onTextMessageReceived(QString message) {
    emit logMessage("Message received: " + message);
    qDebug() << "Message received:" << message;

    // Логирование сообщения в файл
    logMessageToFile(message);

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    QJsonObject obj = doc.object();

    if (obj.contains("type")) {
        QString type = obj["type"].toString();
        if (type == "result") {
            qDebug() << "Results received from server:" << obj["content"].toObject();
            emit resultsReceived(obj["content"].toObject()); // Эмитирование сигнала с полученными данными
        } else if (type == "answer") {
            qDebug() << "Answer received from server.";
            emit logMessage("Answer received: " + obj["msg"].toString());
        } else if (type == "progress_bar") {
            qDebug() << "Progress bar update received:" << obj["content"].toInt();
            emit logMessage("Progress: " + QString::number(obj["content"].toInt()) + "%");
        } else {
            qDebug() << "Unknown message type received:" << type;
            qDebug() << "Message content:" << message;
        }
    } else {
        qDebug() << "Message does not contain a type field.";
        qDebug() << "Message content:" << message;
    }
}

// Функция для авторизации на сервере
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

// Функция для отправки данных о треугольниках на сервер
void TriangleClient::sendTriangleData(const QVector<QSharedPointer<triangle>>& triangles) {
    if (!m_webSocket->isValid()) {
        qDebug() << "WebSocket is not connected. Attempting to resend data...";
        QTimer::singleShot(5000, this, [this, triangles]() { sendTriangleData(triangles); });
        return;
    }

    QJsonObject dataObject;
    int index = 0;
    for (const auto& tri : triangles) {
        if (tri->getVisible()) {
            dataObject[QString::number(index)] = vectorToJson(tri->getV1());
            dataObject[QString::number(index + 1)] = vectorToJson(tri->getV2());
            dataObject[QString::number(index + 2)] = vectorToJson(tri->getV3());
            index += 3;
        }
    }

    QJsonObject messageObject = {
        {"type", "triangles"},
        {"data", dataObject},
        {"polarRadiation", m_polarRadiation},
        {"polarRecive", m_polarRecive},
        {"typeAngle", m_typeAngle},
        {"typeAzimut", m_typeAzimut},
        {"typeLength", m_typeLength}
    };
    QJsonDocument doc(messageObject);
    QString message = doc.toJson(QJsonDocument::Compact);

    qDebug() << "Sending triangle data to server:" << message;

    if (m_webSocket->sendTextMessage(message) == -1) {
        qDebug() << "Error sending triangle data to server";
        emit logMessage("Error sending triangle data to server");
    } else {
        qDebug() << "Sending triangle data to server...";
    }
}

// Функция для отправки команды на сервер
void TriangleClient::sendCommand(const QJsonObject &commandObject) {
    if (m_webSocket->isValid()) {
        QJsonObject messageObject = commandObject;

        // Добавляем вектор направления
        QJsonObject directVector;
        directVector["x"] = m_directVector.getX();
        directVector["y"] = m_directVector.getY();
        directVector["z"] = m_directVector.getZ();
        messageObject["directVector"] = directVector;

        QJsonDocument doc(messageObject);
        QString dataString = doc.toJson(QJsonDocument::Compact);
        qDebug() << "Sending data to server:" << dataString;

        if (m_webSocket->sendTextMessage(dataString) == -1) {
            qDebug() << "Error sending command to server";
            emit logMessage("Error sending command to server");
        } else {
            qDebug() << "Sending command to server...";
        }
    }
}

void TriangleClient::setDirectVector(const rVect& directVector) {
    m_directVector = directVector;
}

// Установите значения для поляризации и типов радиопортрета
void TriangleClient::setPolarizationAndType(int polarRadiation, int polarRecive, bool typeAngle, bool typeAzimut, bool typeLength) {
    m_polarRadiation = polarRadiation;
    m_polarRecive = polarRecive;
    m_typeAngle = typeAngle;
    m_typeAzimut = typeAzimut;
    m_typeLength = typeLength;
}

// Преобразование вектора в JSON объект
QJsonObject TriangleClient::vectorToJson(const QSharedPointer<const rVect>& vector) {
    return {
        {"x", vector->getX()},
        {"y", vector->getY()},
        {"z", vector->getZ()}
    };
}

// Отправка данных модели на сервер
void TriangleClient::sendModelData(const QJsonObject &modelData) {
    if (!m_webSocket->isValid()) {
        qDebug() << "WebSocket is not connected. Attempting to resend model data...";
        QTimer::singleShot(5000, this, [this, modelData]() { sendModelData(modelData); });
        return;
    }

    QJsonDocument doc(modelData);
    QString message = doc.toJson(QJsonDocument::Compact);

    qDebug() << "Sending model data to server:" << message;

    if (m_webSocket->sendTextMessage(message) == -1) {
        qDebug() << "Error sending model data to server";
        emit logMessage("Error sending model data to server");
    } else {
        qDebug() << "Sending model data to server...";
    }
}

// Обработка полученных результатов с сервера
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

void TriangleClient::logMessageToFile(const QString &message) {
    QFile file("received_messages.log");
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream out(&file);
        out << message << "\n";
    }
}
