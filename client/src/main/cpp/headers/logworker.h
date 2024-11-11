#ifndef LOGWORKER_H
#define LOGWORKER_H

#include <QObject>
#include <QQueue>
#include <QMutex>
#include <QMutexLocker>

class LogWorker : public QObject {
    Q_OBJECT
public:
    explicit LogWorker(QObject *parent = nullptr);

public slots:
    void enqueueMessage(const QString &message);
    void processLogQueue();

private:
    QQueue<QString> m_logQueue;
    QMutex m_logMutex;
};

#endif // LOGWORKER_H
