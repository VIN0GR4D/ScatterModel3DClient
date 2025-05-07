#ifndef CONNECTIONMANAGER_H
#define CONNECTIONMANAGER_H

#include <QObject>
#include <QUrl>
#include <QString>
#include <QSharedPointer>
#include <QJsonObject>
#include <QVector>
#include "Triangle.h"
#include "notification.h"

// Предварительные объявления классов
class TriangleClient;
class QWebSocket;

// Интерфейс для обратных вызовов
class IConnectionObserver {
public:
    virtual ~IConnectionObserver() = default;
    virtual void onConnectionStatusChanged(bool connected) = 0;
    virtual void onResultsReceived(const QJsonObject &results) = 0;
    virtual void onCalculationProgress(int progress) = 0;
    virtual void onLogMessage(const QString &message) = 0;
    virtual void onNotification(const QString &message, Notification::Type type) = 0;
    virtual void onCalculationAborted() = 0;
};

class ConnectionManager : public QObject {
    Q_OBJECT

public:
    explicit ConnectionManager(QObject *parent = nullptr);
    ~ConnectionManager();

    // Методы для управления соединением
    void connectToServer(const QString &serverAddress);
    void disconnectFromServer();
    bool isConnected() const;
    bool isAuthorized() const;

    // Методы для авторизации
    void authorize(const QString &username, const QString &password);

    // Методы для отправки данных
    void sendModelData(const QJsonObject &modelData);
    void sendCommand(const QJsonObject &commandObject);
    void abortCalculation();

    // Методы для установки параметров
    void setPolarizationAndType(int polarRadiation, int polarRecive,
                                bool typeAngle, bool typeAzimut, bool typeLength);
    void setDirectVector(const rVect& directVector);

    // Метод для регистрации наблюдателя
    void registerObserver(IConnectionObserver* observer);
    void unregisterObserver(IConnectionObserver* observer);

private:
    std::unique_ptr<TriangleClient> m_triangleClient;
    QVector<IConnectionObserver*> m_observers;

    // Внутренние методы для обработки событий
    void handleConnected();
    void handleDisconnected();
    void handleResultsReceived(const QJsonObject &results);
    void handleCalculationProgress(int progress);
    void handleLogMessage(const QString &message);
    void handleNotification(const QString &message, Notification::Type type);
    void handleCalculationAborted();

    // Вспомогательные методы
    QJsonObject vectorToJson(const QSharedPointer<const rVect>& vector);
};

#endif // CONNECTIONMANAGER_H
