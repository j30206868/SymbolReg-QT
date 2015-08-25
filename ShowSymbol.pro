#-------------------------------------------------
#
# Project created by QtCreator 2015-08-24T18:33:00
#
#-------------------------------------------------

QT += core gui
CONFIG += console

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ShowSymbol
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    renderarea.cpp \
    symboltype.cpp \
    commonfunc.cpp \
    symbolreg.cpp \
    mousectrl.cpp \
    cursor.cpp

HEADERS  += mainwindow.h \
    renderarea.h \
    symboltype.h \
    commonfunc.h \
    symbolreg.h \
    mousectrl.h \
    cursor.h

FORMS    += mainwindow.ui

RESOURCES += \
    basicdrawing.qrc
