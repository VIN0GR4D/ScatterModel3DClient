#include "radar_thread.h"

radar_thread::radar_thread()
{
    mTimer = new QTimer(this);
    connect(this, SIGNAL(start_timer()), this, SLOT(slot_timer_start()));
    connect(mTimer, SIGNAL(timeout()), this, SLOT(slot_send_progress()));
}

void radar_thread::run() {
    emit(start_timer());
    exec();
}

void radar_thread::slot_timer_start(){
    mTimer->start(1000);
    run();
}
