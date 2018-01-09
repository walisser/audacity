TEMPLATE = app
#TARGET = TestXMLValueChecker
QT = core testlib concurrent
CONFIG += precompile_header console
CONFIG -= app_bundle

INCLUDEPATH += . ../

PRECOMPILED_HEADER = ../core/Prefix.h
#QMAKE_CXXFLAGS += -fdiagnostics-color=always

precompile_header:*android* {
    #warning("Android PCH workaround...")
    #warning($$QMAKESPEC)
    #QMAKE_CXXFLAGS += -include $$PRECOMPILED_HEADER
}

!*android* {
    # gcc coverage
    QMAKE_CXXFLAGS += -fprofile-arcs -ftest-coverage -O0
    LIBS += -lgcov
}

DEFINES += BUILDING_AUDACITY

SOURCES += $${TARGET}.cpp \
   ../core/QtUtil.cpp
