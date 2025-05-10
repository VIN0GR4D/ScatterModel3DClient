#include "connectionmanager.h"
#include "triangleclient.h"

ConnectionManager::ConnectionManager(QObject *parent)
    : QObject(parent), m_triangleClient(nullptr)
{
}

ConnectionManager::~ConnectionManager()
{
    // Очищаем список наблюдателей перед уничтожением
    m_observers.clear();

    // Затем уничтожаем m_triangleClient без вызова disconnectFromServer()
    if (m_triangleClient) {
        m_triangleClient.reset();
    }
}

void ConnectionManager::registerObserver(IConnectionObserver* observer)
{
    if (observer && !m_observers.contains(observer)) {
        m_observers.append(observer);
    }
}

void ConnectionManager::unregisterObserver(IConnectionObserver* observer)
{
    m_observers.removeAll(observer);
}

void ConnectionManager::connectToServer(const QString &serverAddress)
{
    if (serverAddress.isEmpty()) {
        handleLogMessage("Укажите адрес сервера.");
        handleNotification("Укажите адрес сервера", Notification::Warning);
        return;
    }

    if (m_triangleClient) {
        m_triangleClient.reset();
    }

    m_triangleClient = std::make_unique<TriangleClient>(QUrl(serverAddress), this);

    if (m_triangleClient) {
        // Соединение сигналов и слотов
        connect(m_triangleClient.get(), &TriangleClient::resultsReceived,
                this, &ConnectionManager::handleResultsReceived);
        connect(m_triangleClient.get(), &TriangleClient::logMessage,
                this, &ConnectionManager::handleLogMessage);
        connect(m_triangleClient.get(), &TriangleClient::showNotification,
                this, &ConnectionManager::handleNotification);
        connect(m_triangleClient.get(), &TriangleClient::progressUpdated,
                this, &ConnectionManager::handleCalculationProgress);
        connect(m_triangleClient.get(), &TriangleClient::calculationAborted,
                this, &ConnectionManager::handleCalculationAborted);

        // Дополнительные соединения для отслеживания статуса подключения
        if (m_triangleClient->getWebSocket()) {
            connect(m_triangleClient->getWebSocket(), &QWebSocket::connected,
                    this, &ConnectionManager::handleConnected);
            connect(m_triangleClient->getWebSocket(), &QWebSocket::disconnected,
                    this, &ConnectionManager::handleDisconnected);
        }

        handleLogMessage("Подключение к серверу: " + serverAddress);
        handleNotification("Подключение к серверу...", Notification::Info);
    } else {
        handleLogMessage("Ошибка создания клиента.");
        handleNotification("Ошибка создания клиента", Notification::Error);
    }
}

void ConnectionManager::disconnectFromServer()
{
    if (m_triangleClient && m_triangleClient->isConnected()) {
        m_triangleClient->disconnectFromServer();
        handleLogMessage("Отключен от сервера.");
    } else {
        handleLogMessage("Не подключен к серверу.");
    }
}

bool ConnectionManager::isConnected() const
{
    return m_triangleClient && m_triangleClient->isConnected();
}

bool ConnectionManager::isAuthorized() const
{
    return m_triangleClient && m_triangleClient->isAuthorized();
}

void ConnectionManager::authorize(const QString &username, const QString &password)
{
    if (m_triangleClient) {
        m_triangleClient->authorize(username, password);
    } else {
        handleLogMessage("Не подключен к серверу.");
        handleNotification("Не подключен к серверу", Notification::Warning);
    }
}

void ConnectionManager::sendModelData(const QJsonObject &modelData)
{
    if (isConnected() && isAuthorized()) {
        m_triangleClient->sendModelData(modelData);
    } else if (isConnected() && !isAuthorized()) {
        handleLogMessage("Требуется авторизация перед отправкой данных.");
        handleNotification("Требуется авторизация", Notification::Warning);
    } else {
        handleLogMessage("Не подключен к серверу.");
        handleNotification("Не подключен к серверу", Notification::Warning);
    }
}

void ConnectionManager::sendCommand(const QJsonObject &commandObject)
{
    if (isConnected()) {
        m_triangleClient->sendCommand(commandObject);
    } else {
        handleLogMessage("Не подключен к серверу.");
        handleNotification("Не подключен к серверу", Notification::Warning);
    }
}

void ConnectionManager::abortCalculation()
{
    if (!isConnected()) {
        handleLogMessage("Нет активного подключения к серверу");
        return;
    }

    QJsonObject commandObject;
    commandObject["type"] = "cmd";
    commandObject["cmd"] = "server";
    QJsonArray params;
    params.append("stop");
    commandObject["param"] = params;

    sendCommand(commandObject);
    handleLogMessage("Отправлена команда прерывания расчёта");
    handleNotification("Расчёт прерван", Notification::Info);
}

void ConnectionManager::setPolarizationAndType(int polarRadiation, int polarRecive,
                                               bool typeAngle, bool typeAzimut, bool typeLength)
{
    if (m_triangleClient) {
        m_triangleClient->setPolarizationAndType(polarRadiation, polarRecive,
                                                 typeAngle, typeAzimut, typeLength);
    }
}

void ConnectionManager::setDirectVector(const rVect& directVector)
{
    if (m_triangleClient) {
        m_triangleClient->setDirectVector(directVector);
    }
}

QJsonObject ConnectionManager::vectorToJson(const QSharedPointer<const rVect>& vector)
{
    QJsonObject obj;
    obj["x"] = vector->getX();
    obj["y"] = vector->getY();
    obj["z"] = vector->getZ();
    return obj;
}

// Обработчики событий
void ConnectionManager::handleConnected()
{
    for (auto observer : m_observers) {
        observer->onConnectionStatusChanged(true);
    }
}

void ConnectionManager::handleDisconnected()
{
    for (auto observer : m_observers) {
        observer->onConnectionStatusChanged(false);
    }
}

void ConnectionManager::handleResultsReceived(const QJsonObject &results)
{
    for (auto observer : m_observers) {
        observer->onResultsReceived(results);
    }
}

void ConnectionManager::handleCalculationProgress(int progress)
{
    for (auto observer : m_observers) {
        observer->onCalculationProgress(progress);
    }
}

void ConnectionManager::handleLogMessage(const QString &message)
{
    // Защита от вызова после того, как объект начал уничтожаться
    if (m_observers.isEmpty()) {
        return;
    }

    for (auto observer : m_observers) {
        if (observer) {
            observer->onLogMessage(message);
        }
    }
}

void ConnectionManager::handleNotification(const QString &message, Notification::Type type)
{
    if (m_observers.isEmpty()) {
        return;
    }

    for (auto observer : m_observers) {
        if (observer) {
            observer->onNotification(message, type);
        }
    }
}

void ConnectionManager::handleCalculationAborted()
{
    for (auto observer : m_observers) {
        observer->onCalculationAborted();
    }
}
