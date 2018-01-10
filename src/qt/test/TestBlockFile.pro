include(TestCommon.pri)

LIBS += -lsndfile -lexpat

# Input
SOURCES += \
   ../core/BlockFile.cpp \
   ../core/SampleFormat.cpp \
   ../core/Dither.cpp \
   ../core/FileException.cpp \
   ../core/AudacityException.cpp \
   ../core/FileFormats.cpp \
   ../core/blockfile/SimpleBlockFile.cpp \
   ../core/xml/XMLTagHandler.cpp \
   ../core/xml/XMLWriter.cpp \
   ../core/xml/XMLFileReader.cpp
