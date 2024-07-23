#include <windows.h>
#include <QCoreApplication>
#include "calctools.h"
#include "webserver.h"
#include <QTextCodec>

QScopedPointer<QFile> m_logFile;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
#ifdef Q_OS_WIN32
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("IBM 866"));
#endif

#ifdef Q_OS_LINUX
    QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
#endif

    m_logFile.reset(new QFile("cslogs.txt"));
    m_logFile.data()->open(QFile::Append | QFile::Text);
    WebServer myServer(1234);

    return a.exec();
}
