#-------------------------------------------------
#
# Project created by QtCreator 2014-03-10T10:31:43
#
#-------------------------------------------------

QT       += core gui network printsupport widgets

TARGET = OdroidReader
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11 -Wall -pedantic -Wextra -fno-strict-aliasing
LIBS += -lhidapi-libusb
SOURCES += main.cpp\
        odroidreader.cpp \
    ipvalidator.cpp \
    experiment.cpp \
    qcustomplot.cpp \
    environmentmodel.cpp \
    environmentdelegate.cpp \
    Data/datadescriptor.cpp \
    Data/datasource.cpp \
    ui/dataexplorer.cpp \
    Sources/networksource.cpp \
    Data/dataseries.cpp \
    Data/statisticalset.cpp \
    ui/tabstyle.cpp \
    Sources/odroidsmartpowersource.cpp \
    devicemonitor.cpp \
    smartpowermonitor.cpp

HEADERS  += odroidreader.h \
    ipvalidator.h \
    experiment.h \
    qcustomplot.h \
    environmentmodel.h \
    environmentdelegate.h \
    Data/datadescriptor.h \
    Data/datasource.h \
    ui/dataexplorer.h \
    Sources/networksource.h \
    Data/dataseries.h \
    Data/statisticalset.h \
    ui/tabstyle.h \
    Sources/odroidsmartpowersource.h \
    devicemonitor.h \
    smartpowermonitor.h

FORMS    += odroidreader.ui \
    ui/dataexplorer.ui

unix:!macx: LIBS += -L$$PWD/../build-qhidapi-Desktop-Debug/ -lqhidapi

INCLUDEPATH += $$PWD/../qhidapi
DEPENDPATH += $$PWD/../qhidapi
