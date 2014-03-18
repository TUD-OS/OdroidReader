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

QMAKE_CXXFLAGS += -std=c++11

SOURCES += main.cpp\
        odroidreader.cpp \
    ipvalidator.cpp \
    value.cpp

HEADERS  += odroidreader.h \
    ipvalidator.h \
    value.h

FORMS    += odroidreader.ui
