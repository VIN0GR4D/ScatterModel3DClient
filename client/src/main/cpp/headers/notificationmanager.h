#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <QObject>
#include <QList>
#include <QHash>
#include <QDateTime>
#include "notification.h"

struct NotificationInfo {
    QString message;
    Notification::Type type;
    QDateTime lastShown;
    int count;
};

class NotificationManager : public QObject
{
    Q_OBJECT
    friend class MainWindow;
public:
    static NotificationManager *instance();
    void showMessage(const QString &message, Notification::Type type = Notification::Info, int duration = 10000);
    static void destroy();

private:
    explicit NotificationManager(QObject *parent = nullptr);
    ~NotificationManager();

    static NotificationManager *s_instance;
    QList<Notification*> m_notifications;
    QHash<QString, NotificationInfo> m_notificationHistory;
    const int SPACING = 10;  // Расстояние между уведомлениями
    const int SPAM_THRESHOLD = 3;  // Количество повторений до блокировки
    const int COOLDOWN_MS = 5000;  // Время остывания в миллисекундах

    bool isSpam(const QString &message, Notification::Type type);
    void updateNotificationHistory(const QString &message, Notification::Type type);
    QString createHistoryKey(const QString &message, Notification::Type type);

private slots:
    void removeNotification(QObject* notification);
    void updatePositions();
};

#endif // NOTIFICATIONMANAGER_H
