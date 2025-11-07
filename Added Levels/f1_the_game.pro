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
    mainwindow.h \
    media.h \
    nitro.h \
    outro.h \
    point.h \
    wheel.h \
    line.h

# List all source files here
SOURCES += \
    carBody.cpp \
    coin.cpp \
    flip.cpp \
    fuel.cpp \
    intro.cpp \
    main.cpp \
    mainwindow.cpp \
    media.cpp \
    nitro.cpp \
    outro.cpp \
    point.cpp \
    wheel.cpp \
    line.cpp
FORMS += \
    mainwindow.ui

RESOURCES += \
    assets.qrc
