include(config_pre.prf)

HEADERS += TestBlockFile.h

SOURCES += \
   ../core/SampleFormat.cpp \
   ../core/Dither.cpp \
   ../core/FileException.cpp \
   ../core/AudacityException.cpp \
   ../core/FileFormats.cpp \
   ../core/xml/XMLTagHandler.cpp \
   ../core/xml/XMLWriter.cpp \
   ../core/xml/XMLFileReader.cpp \
   ../core/BlockFile.cpp \
   ../core/blockfile/SimpleBlockFile.cpp \
   ../core/blockfile/SilentBlockFile.cpp \
   ../core/blockfile/PCMAliasBlockFile.cpp \
   ../core/blockfile/ODPCMAliasBlockFile.cpp \
   ../core/blockfile/ODDecodeBlockFile.cpp \
   ../core/blockfile/NotYetAvailableException.cpp \
   ../core/InconsistencyException.cpp \
   ../core/DialogManager.cpp \
   ../core/DirManager.cpp

addLibrary(sndfile)|error(Failed to find sndfile)
addLibrary(expat)|error(Failed to find expat)

include(config_post.prf)
