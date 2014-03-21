#-------------------------------------------------
#
# Project created by QtCreator 2014-03-10T10:31:43
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = OdroidReader
TEMPLATE = app

CONFIG += qwt

QMAKE_CXXFLAGS += -std=c++11 -Wall -pedantic -Wextra -fno-strict-aliasing

SOURCES += main.cpp\
        odroidreader.cpp \
    ipvalidator.cpp \
    datapoint.cpp \
    experiment.cpp

HEADERS  += odroidreader.h \
    ipvalidator.h \
    value.h \
    datapoint.h \
    experiment.h

FORMS    += odroidreader.ui
