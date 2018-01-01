


TEMPLATE = app
#TARGET = TestXMLValueChecker
INCLUDEPATH += . ../
QT = core testlib concurrent
CONFIG += precompile_header debug
PRECOMPILED_HEADER = ../core/Prefix.h
#QMAKE_CXXFLAGS += -fdiagnostics-color=always
MOC_DIR=build/moc
OBJECTS_DIR=build/obj
DEFINES += BUILDING_AUDACITY

SOURCES += $${TARGET}.cpp \
   ../core/QtUtil.cpp
