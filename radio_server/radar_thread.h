#ifndef RADAR_THREAD_H
#define RADAR_THREAD_H

#include <QTimer>
#include <QThread>
#include <iostream>

class radar_thread : public QThread
{
    Q_OBJECT
public:
    explicit radar_thread();
    QTimer *mTimer;
signals:
   void start_timer();
public slots:
    void slot_timer_start();
    void slot_send_progress() {
        std::cout << "Work";
    }
protected:
    void run();
};

#endif // RADAR_THREAD_H
