QT       += core gui websockets opengl openglwidgets widgets charts printsupport 3dcore 3drender 3dinput 3dextras 3dquickextras
LIBS += -lopengl32
LIBS += -lglu32

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets charts

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    aboutdialog.cpp \
    graphwindow.cpp \
    logindialog.cpp \
    logworker.cpp \
    main.cpp \
    mainwindow.cpp \
    meshfilter.cpp \
    newprojectdialog.cpp \
    notification.cpp \
    openglwidget.cpp \
    parser.cpp \
    portraitwindow.cpp \
    projectserializer.cpp \
    raytracer.cpp \
    resultsdialog.cpp \
    triangleclient.cpp

HEADERS += \
    ConstAndVar.h \
    Node.h \
    Triangle.h \
    aboutdialog.h \
    cVect.h \
    graphwindow.h \
    logindialog.h \
    logworker.h \
    mainwindow.h \
    meshfilter.h \
    newprojectdialog.h \
    notification.h \
    openglwidget.h \
    parser.h \
    portraitwindow.h \
    projectserializer.h \
    rVect.h \
    raytracer.h \
    resultsdialog.h \
    triangleclient.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    Car.obj \
    Cube.obj \
    CubeNew.obj \
    Cylindr.obj \
    HollowCylinder.obj \
    Sph.obj \
    apply.png \
    bmw.obj \
    calculator.png \
    cat.obj \
    chair.obj \
    connect.png \
    dark-theme.png \
    darktheme.qss \
    disc.obj \
    disconnect.png \
    download.png \
    error.png \
    footballPlayer.obj \
    heart.obj \
    helicopter.obj \
    icon.png \
    information.png \
    light-theme.png \
    lighttheme.qss \
    load.png \
    login-icon.png \
    m4a1.obj \
    plane.obj \
    reset.png \
    simple_triangle.obj \
    styles.css \
    success.png \
    t34.obj \
    test.obj \
    warning.png

RESOURCES += \
    resources.qrc
