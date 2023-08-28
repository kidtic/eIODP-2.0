QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11


HEADERS += \
    $$PWD/qiodp.h \
    $$PWD/../../../src/eiodp.h

SOURCES += \
    $$PWD/qiodp.cpp \
    $$PWD/../../../src/eiodp.c

INCLUDEPATH += $$PWD/../../../src \
                $$PWD
