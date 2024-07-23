QT -= gui
QT = websockets
QT += testlib
QT += core5compat

CONFIG += c++11 console
CONFIG -= app_bundle
CONFIG(release, debug|release):DEFINES += QT_NO_DEBUG_OUTPUT

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        Calc_Radar/CulcRadar.cpp \
        Calc_Radar/Radar_Wave.cpp \
        calctools.cpp \
        clientai.cpp \
        main.cpp \
        radar_core.cpp \
        webserver.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    Calc_Radar/CPUFFT.h \
    Calc_Radar/ConstAndVar.h \
    Calc_Radar/CulcRadar.h \
    Calc_Radar/Edge.h \
    Calc_Radar/Node.h \
    Calc_Radar/Radar_Wave.h \
    Calc_Radar/Triangle.h \
    Calc_Radar/VectFFT.h \
    Calc_Radar/cVect.h \
    Calc_Radar/rMatrix.h \
    Calc_Radar/rVect.h \
    calctools.h \
    clientai.h \
    radar_core.h \
    timer.h \
    webserver.h
