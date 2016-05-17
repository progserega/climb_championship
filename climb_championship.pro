#-------------------------------------------------
#
# Project created by QtCreator 2016-05-08T23:47:14
#
#-------------------------------------------------

QT       += core gui serialport multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = climb_championship
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp


HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

TRANSLATIONS += climb_championship_ru.ts

OTHER_FILES += \
    ring2.wav \
    ring1.wav \
    climb_championship_ru.ts
