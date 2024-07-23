#ifndef QSLEEP_H
#define QSLEEP_H

#include <QTest>

#ifdef Q_OS_WIN
#include <windows.h> // for Sleep
#endif

void QTest::qSleep(int ms)
{
    assert(ms > 0);

#ifdef Q_OS_WIN
    Sleep(uint(ms));
#else
    struct timespec ts = { ms / 1000, (ms % 1000) * 1000 * 1000 };
    nanosleep(&ts, NULL);
#endif
}

#endif // QSLEEP_H
