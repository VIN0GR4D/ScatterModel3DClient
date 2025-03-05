#include "logworker.h"
#include <QFile>
#include <QTextStream>
#include <QFileInfo>
#include <QDateTime>
#include <QDir>

LogWorker::LogWorker(QObject *parent) : QObject(parent) {}

void LogWorker::setMaxQueueSize(int size) {
    if (size > 0) {
        QMutexLocker locker(&m_logMutex);
        m_maxQueueSize = size;
    }
}

void LogWorker::setMaxLogFileSize(qint64 size) {
    if (size > 0) {
        QMutexLocker locker(&m_logMutex);
        m_maxLogFileSize = size;
    }
}

void LogWorker::enqueueMessage(const QString &message) {
    QMutexLocker locker(&m_logMutex);

    // Проверяем, не превышает ли размер очереди максимальное значение
    if (m_logQueue.size() >= m_maxQueueSize) {
        // Удаляем самое старое сообщение
        m_logQueue.dequeue();
    }

    m_logQueue.enqueue(message);
}

bool LogWorker::rotateLogFileIfNeeded(const QString &filePath) {
    QFileInfo fileInfo(filePath);

    // Если файл существует и превышает максимальный размер
    if (fileInfo.exists() && fileInfo.size() >= m_maxLogFileSize) {
        // Формируем имя для архивного файла с датой и временем
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        QString archiveFileName = fileInfo.path() + QDir::separator() +
                                  fileInfo.baseName() + "_" + timestamp + "." +
                                  fileInfo.suffix();

        // Закрываем текущий файл (если был открыт)
        QFile oldFile(filePath);
        if (oldFile.isOpen()) {
            oldFile.close();
        }

        // Переименовываем текущий файл
        return QFile::rename(filePath, archiveFileName);
    }

    return true;
}

void LogWorker::processLogQueue() {
    const QString logFilePath = "received_messages.log";

    // Проверяем и при необходимости ротируем лог-файл
    rotateLogFileIfNeeded(logFilePath);

    QFile file(logFilePath);
    bool fileOpened = false;

    if (file.open(QIODevice::Append | QIODevice::Text)) {
        fileOpened = true;
    }

    QTextStream out(&file);

    while (true) {
        QString message;
        {
            QMutexLocker locker(&m_logMutex);
            if (m_logQueue.isEmpty()) {
                break;
            }
            message = m_logQueue.dequeue();
        }

        if (fileOpened) {
            out << message << "\n";
            // Сбрасываем буфер для немедленной записи
            out.flush();
        }
    }

    if (fileOpened) {
        file.close();
    }
}
