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
   ../core/ondemand/ODDecodeTask.cpp \
   ../core/blockfile/SimpleBlockFile.cpp \
   ../core/blockfile/NotYetAvailableException.cpp \
   ../core/blockfile/ODDecodeBlockFile.cpp \
   ../core/ondemand/ODTask.cpp \
   ../core/ondemand/ODDecodeFlacTask.cpp

addLibrary(sndfile)|error(Failed to find sndfile)
addLibrary(expat)|error(Failed to find expat)
addLibrary(flac++,libflac)|error(Failed to find flac++)

include(config_post.prf)
