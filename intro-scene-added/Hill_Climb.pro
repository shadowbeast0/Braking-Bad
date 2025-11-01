# driver.pro

QT       += core gui widgets
CONFIG   += c++17

TARGET = driver
TEMPLATE = app

# List all header files here
HEADERS += \
    intro.h \
    mainwindow.h \
    wheel.h \
    line.h

# List all source files here
SOURCES += \
    intro.cpp \
    main.cpp \
    mainwindow.cpp \
    wheel.cpp \
    line.cpp
FORMS += \
    mainwindow.ui
