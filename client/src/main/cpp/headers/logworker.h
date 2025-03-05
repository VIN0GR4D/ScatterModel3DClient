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

    // Установка максимального размера очереди
    void setMaxQueueSize(int size);

    // Установка максимального размера файла лога (в байтах)
    void setMaxLogFileSize(qint64 size);

public slots:
    void enqueueMessage(const QString &message);
    void processLogQueue();

private:
    QQueue<QString> m_logQueue;
    QMutex m_logMutex;
    int m_maxQueueSize = 1000; // Ограничение по умолчанию
    qint64 m_maxLogFileSize = 10 * 1024 * 1024; // 10 МБ по умолчанию

    // Метод для ротации лог-файла
    bool rotateLogFileIfNeeded(const QString &filePath);
};

#endif // LOGWORKER_H
