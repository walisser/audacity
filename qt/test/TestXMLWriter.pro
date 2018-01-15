include(config_pre.prf)

SOURCES += \
   ../core/xml/XMLWriter.cpp \
   ../core/xml/XMLFileReader.cpp \
   ../core/xml/XMLTagHandler.cpp \
   ../core/FileException.cpp \
   ../core/AudacityException.cpp \

addLibrary(expat)|error(This test requires expat)

include(config_post.prf)
