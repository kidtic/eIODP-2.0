QT       += core gui network serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11


HEADERS += \
    $$PWD/qiodp.h \
    $$PWD/../../eiodp.h

SOURCES += \
    $$PWD/qiodp.cpp \
    $$PWD/../../eiodp.c

INCLUDEPATH += $$PWD/../../ \
               $$PWD
