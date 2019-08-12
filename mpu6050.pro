#-------------------------------------------------
#
# Project created by QtCreator 2017-04-10T17:33:47
#
#-------------------------------------------------

QT       += core gui
QT += serialport
QT += widgets printsupport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = serialport
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    qcustomplot.cpp

HEADERS  += mainwindow.h \
    qcustomplot.h

FORMS    += mainwindow.ui
