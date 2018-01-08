include(TestCommon.pri)

LIBS += -lexpat

# Input
SOURCES += \
   ../core/xml/XMLWriter.cpp \
   ../core/xml/XMLFileReader.cpp \
   ../core/xml/XMLTagHandler.cpp \
   ../core/FileException.cpp \
   ../core/AudacityException.cpp \
