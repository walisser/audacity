include(config_pre.prf)

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

addLibrary(sndfile)|error(Failed to find sndfile)
addLibrary(expat)|error(Failed to find expat)

include(config_post.prf)
