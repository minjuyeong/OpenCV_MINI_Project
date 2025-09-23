QT       += core gui widgets multimedia multimediawidgets websockets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17 link_pkgconfig

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0
PKGCONFIG += opencv4

SOURCES += \
    main.cpp \
    mainwidget.cpp \
    motiondetector.cpp \
    streamserver.cpp \
    tab1_camera.cpp \
    tab2_video.cpp

HEADERS += \
    mainwidget.h \
    motiondetector.h \
    streamserver.h \
    tab1_camera.h \
    tab2_video.h

FORMS += \
    mainwidget.ui \
    tab1_camera.ui \
    tab2_video.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
