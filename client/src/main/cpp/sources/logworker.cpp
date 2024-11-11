#include "logworker.h"
#include <QFile>
#include <QTextStream>

LogWorker::LogWorker(QObject *parent) : QObject(parent) {}

void LogWorker::enqueueMessage(const QString &message) {
    QMutexLocker locker(&m_logMutex);
    m_logQueue.enqueue(message);
}

void LogWorker::processLogQueue() {
    while (true) {
        QString message;
        {
            QMutexLocker locker(&m_logMutex);
            if (m_logQueue.isEmpty()) {
                break;
            }
            message = m_logQueue.dequeue();
        }

        QFile file("received_messages.log");
        if (file.open(QIODevice::Append | QIODevice::Text)) {
            QTextStream out(&file);
            out << message << "\n";
        }
    }
}
