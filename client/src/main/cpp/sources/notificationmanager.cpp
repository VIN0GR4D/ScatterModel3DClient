#include "notificationmanager.h"
#include <QApplication>
#include <QScreen>

NotificationManager* NotificationManager::s_instance = nullptr;

NotificationManager::NotificationManager(QObject *parent)
    : QObject(parent)
{
}

NotificationManager::~NotificationManager()
{
    qDeleteAll(m_notifications);
}

NotificationManager *NotificationManager::instance()
{
    if (!s_instance) {
        s_instance = new NotificationManager(qApp);
    }
    return s_instance;
}

void NotificationManager::destroy()
{
    if (s_instance) {
        delete s_instance;
        s_instance = nullptr;
    }
}

QString NotificationManager::createHistoryKey(const QString &message, Notification::Type type)
{
    return QString("%1_%2").arg(message).arg(static_cast<int>(type));
}

bool NotificationManager::isSpam(const QString &message, Notification::Type type)
{
    QString key = createHistoryKey(message, type);
    if (!m_notificationHistory.contains(key)) {
        return false;
    }

    const NotificationInfo &info = m_notificationHistory[key];
    QDateTime currentTime = QDateTime::currentDateTime();
    qint64 msSinceLastNotification = info.lastShown.msecsTo(currentTime);

    // Если уведомление показывалось часто и не прошло время остывания
    if (info.count >= SPAM_THRESHOLD && msSinceLastNotification < COOLDOWN_MS) {
        return true;
    }

    // Если прошло достаточно времени, сбрасываем счетчик
    if (msSinceLastNotification >= COOLDOWN_MS) {
        return false;
    }

    return false;
}

void NotificationManager::updateNotificationHistory(const QString &message, Notification::Type type)
{
    QString key = createHistoryKey(message, type);
    QDateTime currentTime = QDateTime::currentDateTime();

    if (m_notificationHistory.contains(key)) {
        NotificationInfo &info = m_notificationHistory[key];
        qint64 msSinceLastNotification = info.lastShown.msecsTo(currentTime);

        // Сбрасываем счетчик, если прошло достаточно времени
        if (msSinceLastNotification >= COOLDOWN_MS) {
            info.count = 1;
        } else {
            info.count++;
        }
        info.lastShown = currentTime;
    } else {
        m_notificationHistory[key] = {message, type, currentTime, 1};
    }
}

void NotificationManager::showMessage(const QString &message, Notification::Type type, int duration)
{
    // Проверяем на спам
    if (isSpam(message, type)) {
        return;
    }

    // Обновляем историю уведомлений
    updateNotificationHistory(message, type);

    // Находим главное окно приложения - MainWindow
    QWidget *mainWindow = QApplication::activeWindow();
    if (!mainWindow) {
        // Если не найдено активное окно, ищем любое окно верхнего уровня
        QWidgetList topLevelWidgets = QApplication::topLevelWidgets();
        for (QWidget *widget : topLevelWidgets) {
            if (widget->isVisible() && widget->windowType() == Qt::Window) {
                mainWindow = widget;
                break;
            }
        }
    }

    // Создаем уведомление с указанием родителя
    Notification *notification = new Notification(mainWindow);
    notification->setAttribute(Qt::WA_DeleteOnClose, true);  // Гарантируем удаление при закрытии

    connect(notification, &QObject::destroyed, this, &NotificationManager::removeNotification);
    connect(notification, &Notification::startedMoving, this, &NotificationManager::updatePositions);

    m_notifications.append(notification);

    int totalHeight = 0;

    for (int i = m_notifications.size() - 2; i >= 0; --i) {
        Notification *prev = m_notifications[i];
        totalHeight += prev->height() + SPACING;
        prev->moveDown(totalHeight);
    }

    notification->showMessage(message, type, duration);
}

void NotificationManager::removeNotification(QObject* notification)
{
    m_notifications.removeOne(static_cast<Notification*>(notification));
    updatePositions();
}

void NotificationManager::updatePositions()
{
    int totalHeight = 0;
    for (int i = m_notifications.size() - 1; i >= 0; --i) {
        Notification *notif = m_notifications[i];
        notif->updatePosition(totalHeight);
        totalHeight += notif->height() + SPACING;
    }
}
