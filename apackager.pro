#-------------------------------------------------
#
# Project created by QtCreator 2012-12-12T20:34:12
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = apackager
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    mpkgmodel.cpp \
    mpkgengine.cpp \
    commitdialog.cpp \
    progresswidget.cpp \
    mpkghandler.cpp \
    mpkgthread.cpp \
    settings.cpp

HEADERS  += mainwindow.h \
    mpkgmodel.h \
    mpkgengine.h \
    commitdialog.h \
    progresswidget.h \
    mpkghandler.h \
    mpkgthread.h \
    settings.h

FORMS    += mainwindow.ui \
    commitdialog.ui \
    progresswidget.ui \
    settings.ui

INCLUDEPATH += /usr/include/libxml2
LIBS += -lmpkg

RESOURCES += \
    resources.qrc

