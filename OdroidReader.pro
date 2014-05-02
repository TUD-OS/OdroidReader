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
    experiment.cpp \
    qcustomplot.cpp \
    environmentmodel.cpp \
    environmentdelegate.cpp \
    Data/datadescriptor.cpp \
    Data/datapoint.cpp \
    Data/datasource.cpp \
    ui/dataexplorer.cpp \
    Sources/networksource.cpp

HEADERS  += odroidreader.h \
    ipvalidator.h \
    value.h \
    experiment.h \
    qcustomplot.h \
    simplevalue.h \
    environmentmodel.h \
    environmentdelegate.h \
    Data/datadescriptor.h \
    Data/datapoint.h \
    Data/datasource.h \
    ui/dataexplorer.h \
    Sources/networksource.h

FORMS    += odroidreader.ui \
    ui/dataexplorer.ui
