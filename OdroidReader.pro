#-------------------------------------------------
#
# Project created by QtCreator 2014-03-10T10:31:43
#
#-------------------------------------------------

QT       += core gui network printsupport widgets

TARGET = OdroidReader
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11 -Wall -pedantic -Wextra -fno-strict-aliasing

SOURCES += main.cpp\
        odroidreader.cpp \
    ipvalidator.cpp \
    datapoint.cpp \
    experiment.cpp \
    qcustomplot.cpp \
    environmentmodel.cpp \
    environmentdelegate.cpp \
    dataexplorer.cpp \
    datasource.cpp

HEADERS  += odroidreader.h \
    ipvalidator.h \
    value.h \
    datapoint.h \
    experiment.h \
    qcustomplot.h \
    simplevalue.h \
    environmentmodel.h \
    environmentdelegate.h \
    dataexplorer.h \
    datasource.h

FORMS    += odroidreader.ui \
    dataexplorer.ui
