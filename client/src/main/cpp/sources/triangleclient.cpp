#include "triangleclient.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QDebug>
#include <QTimer>
#include <QFile>
#include <QtConcurrent/QtConcurrent>

TriangleClient::TriangleClient(const QUrl &url, QObject *parent)
    : QObject(parent),
    m_webSocket(std::make_unique<QWebSocket>()),
    m_url(url),
    m_reconnectAttempts(0),
    m_maxReconnectAttempts(3),
    m_intentionalDisconnect(false),
    m_logWorker(new LogWorker){
    connect(m_webSocket.get(), &QWebSocket::connected, this, &TriangleClient::onConnected);
    connect(m_webSocket.get(), &QWebSocket::disconnected, this, &TriangleClient::onDisconnected);
    connect(m_webSocket.get(), &QWebSocket::textMessageReceived, this, &TriangleClient::onTextMessageReceived);
    connect(m_webSocket.get(), &QWebSocket::errorOccurred, this, &TriangleClient::onErrorOccurred);

    // connect(this, &TriangleClient::logToFile, this, &TriangleClient::logMessageToFile);

    // Настройка LogWorker и потока
    m_logWorker->moveToThread(&m_logThread);
    connect(this, &TriangleClient::logToFile, m_logWorker, &LogWorker::enqueueMessage);
    connect(&m_logThread, &QThread::finished, m_logWorker, &QObject::deleteLater);
    m_logThread.start();

    // Периодический запуск обработки очереди логов
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, m_logWorker, &LogWorker::processLogQueue);
    timer->start(100); // Интервал можно настроить по необходимости

    m_webSocket->open(url);
}

TriangleClient::~TriangleClient() {
    if (m_webSocket->isValid()) {
        m_webSocket->close();
    }
    m_logThread.quit();
    m_logThread.wait();
}

// Попытка переподключения
void TriangleClient::attemptReconnect() {
    if (m_intentionalDisconnect) {
        emit logMessage("Намеренное отключение. Попытки повторного подключения прекращены.");
        return;
    }

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
    emit showNotification("Подключено к серверу", Notification::Success);
    qDebug() << "WebSocket connected, preparing to send data...";
    m_reconnectAttempts = 0;
    m_intentionalDisconnect = false; // сброс флага при успешном подключении
}

bool TriangleClient::isConnected() const {
    return m_webSocket->isValid() && m_webSocket->state() == QAbstractSocket::ConnectedState;
}

void TriangleClient::disconnectFromServer() {
    if (m_webSocket->isValid()) {
        m_intentionalDisconnect = true;
        m_webSocket->close();
        m_isAuthorized = false;
    } else {
        emit logMessage("WebSocket is not connected.");
    }
    m_intentionalDisconnect = false; // сброс флага
}

// Обработчик события отключения от сервера
void TriangleClient::onDisconnected() {
    emit logMessage("WebSocket disconnected");
    emit showNotification("Отключено от сервера", Notification::Warning);
    if (!m_intentionalDisconnect) {
        qDebug() << "WebSocket disconnected, attempting to reconnect...";
        attemptReconnect();
    } else {
        qDebug() << "WebSocket intentionally disconnected.";
    }
}

// Обработчик ошибок соединения
void TriangleClient::onErrorOccurred(QAbstractSocket::SocketError error) {
    emit logMessage("WebSocket error occurred, code: " + QString::number(error));
    emit showNotification("Ошибка подключения", Notification::Error);
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
    // Логируем информацию о полученном сообщении без его полного содержания
    emit logMessage("Получено сообщение. Размер: " + QString::number(message.size()) + " байт.");
    qDebug() << "Получено сообщение. Размер:" << message.size() << " байт.";

    // Асинхронная запись в лог
    emit logToFile(message);

    // Переносим разбор и обработку JSON в отдельный поток
    QtConcurrent::run([this, message]() {
        parseAndProcessMessage(message);
    });
}

// Метод для разбора и обработки полученного сообщения
void TriangleClient::parseAndProcessMessage(const QString& message) {
    // Преобразуем строку в QByteArray для минимизации копирований
    QByteArray messageData = message.toUtf8();

    // Попытка разобрать JSON-документ
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(messageData, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        // Ошибка при разборе JSON
        qDebug() << "Ошибка разбора JSON:" << parseError.errorString();
        // Используем QMetaObject::invokeMethod для вызова в основном потоке
        QMetaObject::invokeMethod(this, [this, parseError]() {
                emit logMessage("Ошибка разбора JSON: " + parseError.errorString());
            }, Qt::QueuedConnection);
        return;
    }

    // Проверка, является ли JSON объектом
    if (!doc.isObject()) {
        qDebug() << "Полученный JSON не является объектом.";
        // Используем QMetaObject::invokeMethod для вызова в основном потоке
        QMetaObject::invokeMethod(this, [this]() {
                emit logMessage("Полученный JSON не является объектом.");
            }, Qt::QueuedConnection);
        return;
    }

    // Получаем JSON-объект
    QJsonObject obj = doc.object();

    // Проверка наличия поля "type" и его строкового типа
    if (obj.contains("type") && obj["type"].isString()) {
        QString type = obj["type"].toString();

        // Обработка типа "result" с содержимым-объектом
        if (type == "result" && obj["content"].isObject()) {
            QJsonObject content = obj["content"].toObject();
            // qDebug() << "Получены результаты от сервера:" << content;
            // Используем QMetaObject::invokeMethod для передачи данных в основной поток
            QMetaObject::invokeMethod(this, [this, content]() {
                    emit resultsReceived(content);
                }, Qt::QueuedConnection);
        }
        // Обработка типа "answer" с текстовым сообщением
        else if (type == "answer" && obj.contains("msg") && obj["msg"].isString()) {
            QString msg = obj["msg"].toString();
            if (msg == "клиент подключен") {
                qDebug() << "Авторизация успешна";
                // Переносим изменение m_isAuthorized в основной поток
                QMetaObject::invokeMethod(this, [this]() {
                        m_isAuthorized = true;
                        emit logMessage("Авторизация успешна.");
                    }, Qt::QueuedConnection);
            } else {
                // Используем QMetaObject::invokeMethod для обновления UI в основном потоке
                QMetaObject::invokeMethod(this, [this, msg]() {
                        emit logMessage("Получен ответ: " + msg);
                    }, Qt::QueuedConnection);
            }
            qDebug() << "Ответ получен от сервера.";
        }
        // Обработка типа "progress_bar" с числовым значением
        else if (type == "progress_bar" && obj.contains("content") && obj["content"].isDouble()) {
            int progress = obj["content"].toInt();
            qDebug() << "Получено обновление прогресс-бара:" << progress;
            // Используем QMetaObject::invokeMethod для обновления прогресса в основном потоке
            QMetaObject::invokeMethod(this, [this, progress]() {
                    emit logMessage("Прогресс: " + QString::number(progress) + "%");
                }, Qt::QueuedConnection);
        }
        // Обработка неизвестного или некорректного типа сообщения
        else {
            qDebug() << "Получен неизвестный или некорректный тип сообщения:" << type;
            // Используем QMetaObject::invokeMethod для вывода сообщения в основном потоке
            QMetaObject::invokeMethod(this, [this, type]() {
                    emit logMessage("Неизвестный или некорректный тип сообщения: " + type);
                }, Qt::QueuedConnection);
        }
    }
    // Сообщение не содержит допустимого поля "type"
    else {
        qDebug() << "Сообщение не содержит допустимого поля type.";
        // Используем QMetaObject::invokeMethod для вывода сообщения в основном потоке
        QMetaObject::invokeMethod(this, [this]() {
                emit logMessage("Сообщение не содержит допустимого поля type.");
            }, Qt::QueuedConnection);
    }
}

// Функция для авторизации на сервере
void TriangleClient::authorize(const QString &username, const QString &password) {
    if (m_isAuthorized) {
        emit logMessage("Already authorized.");
        return;
    }

    QJsonObject authObject = {
        {"type", "auth"},
        {"login", username},
        {"password", password}
    };
    QJsonDocument doc(authObject);
    m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
    qDebug() << "Attempting to authorize...";
}

bool TriangleClient::isAuthorized() const {
    return m_isAuthorized;
}

// Функция для отправки данных о треугольниках на сервер
void TriangleClient::sendTriangleData(const QVector<QSharedPointer<triangle>>& triangles) {
    if (!m_webSocket->isValid()) {
        emit showNotification("Ошибка отправки данных", Notification::Error);
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
        emit showNotification("Ошибка отправки данных на сервер", Notification::Error);
        qDebug() << "Error sending triangle data to server";
        emit logMessage("Error sending triangle data to server");
    } else {
        emit showNotification("Данные успешно отправлены", Notification::Info);
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

    QJsonObject messageObject = modelData;
    messageObject["type"] = "triangles";

    QJsonDocument doc(messageObject);
    QString message = doc.toJson(QJsonDocument::Compact);

    // Добавим отладочное сообщение для проверки данных перед отправкой
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
