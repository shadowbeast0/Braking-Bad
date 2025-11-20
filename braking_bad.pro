# driver.pro

QT       += core gui widgets multimedia
CONFIG   += c++17

TARGET = driver
TEMPLATE = app

# List all header files here
HEADERS += \
    carBody.h \
    coin.h \
    constants.h \
    flip.h \
    fuel.h \
    intro.h \
    keylog.h \
    mainwindow.h \
    media.h \
    nitro.h \
    outro.h \
    pause.h \
    point.h \
    prop.h \
    wheel.h \
    line.h \
    scoreboard.h

# List all source files here
SOURCES += \
    carBody.cpp \
    coin.cpp \
    flip.cpp \
    fuel.cpp \
    intro.cpp \
    keylog.cpp \
    main.cpp \
    mainwindow.cpp \
    media.cpp \
    nitro.cpp \
    outro.cpp \
    pause.cpp \
    point.cpp \
    prop.cpp \
    wheel.cpp \
    line.cpp \
    scoreboard.cpp

FORMS += \
    mainwindow.ui

RESOURCES += \
    assets.qrc
