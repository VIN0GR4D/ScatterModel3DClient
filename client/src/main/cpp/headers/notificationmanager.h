#ifndef NOTIFICATIONMANAGER_H
#define NOTIFICATIONMANAGER_H

#include <QObject>
#include <QList>
#include "notification.h"

class NotificationManager : public QObject
{
    Q_OBJECT
    friend class MainWindow;

public:
    static NotificationManager *instance();
    void showMessage(const QString &message, Notification::Type type = Notification::Info, int duration = 10000);

private:
    explicit NotificationManager(QObject *parent = nullptr);
    ~NotificationManager();

    static NotificationManager *s_instance;
    QList<Notification*> m_notifications;
    const int SPACING = 10;  // Расстояние между уведомлениями

private slots:
    void removeNotification(QObject* notification);
    void updatePositions();
};

#endif // NOTIFICATIONMANAGER_H
