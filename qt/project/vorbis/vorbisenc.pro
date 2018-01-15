include(../config.prf)

TEMPLATE=lib
#TARGET=vorbis
QT=
CONFIG += staticlib
DEFINES += HAVE_CONFIG_H

ROOT=$$LIBSRC/libvorbis

INCLUDEPATH += $$ROOT/include $$ROOT/lib

lib_SOURCES = \
   vorbisenc.c

# add sources with full path
for (SRC, lib_SOURCES) {
   SOURCES += $$ROOT/lib/$$SRC
}

addLibrary(ogg)|error(Failed to find ogg)

endProject()
