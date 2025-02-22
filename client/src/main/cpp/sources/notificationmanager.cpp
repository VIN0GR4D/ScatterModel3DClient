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

void NotificationManager::showMessage(const QString &message, Notification::Type type, int duration)
{
    Notification *notification = new Notification();

    // Подключаем сигнал уничтожения уведомления
    connect(notification, &QObject::destroyed, this, &NotificationManager::removeNotification);
    connect(notification, &Notification::startedMoving, this, &NotificationManager::updatePositions);

    m_notifications.append(notification);

    // Вычисляем позицию для нового уведомления
    QScreen *screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    int x = screenGeometry.width() - notification->width() - 20;
    int totalHeight = 0;

    // Сдвигаем существующие уведомления вниз
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
