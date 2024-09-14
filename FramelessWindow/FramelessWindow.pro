QT += widgets

TEMPLATE = lib
DEFINES += FRAMELESSWINDOW_LIBRARY

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include($$PWD/../global.pri)

SOURCES += \
    framelesswindow.cpp \

HEADERS += \
    FramelessWindow_global.h \
    framelesswindow.h \
    framelesswindow_p.h

win32 {
    SOURCES += framelesswindow_win.cpp
}
unix: !macx {
    LIBS += -lX11 -lXext -lxcb-xinput

    SOURCES += framelesswindow_unix.cpp
}
macx {
    SOURCES += framelesswindow_mac.mm
}

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target
