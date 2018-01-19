include(config_pre.prf)

HEADERS += TestBlockFile.h

SOURCES += \
   TestBlockFile.cpp \
   ../core/BlockFile.cpp \
   ../core/SampleFormat.cpp \
   ../core/Dither.cpp \
   ../core/FileException.cpp \
   ../core/AudacityException.cpp \
   ../core/FileFormats.cpp \
   ../core/xml/XMLTagHandler.cpp \
   ../core/xml/XMLWriter.cpp \
   ../core/xml/XMLFileReader.cpp \
   ../core/blockfile/SilentBlockFile.cpp

addLibrary(sndfile)|error(Failed to find sndfile)
addLibrary(expat)|error(Failed to find expat)

include(config_post.prf)
