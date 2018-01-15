include(../config.prf)

TEMPLATE=lib
#TARGET=vorbis
QT=
CONFIG += staticlib
DEFINES += HAVE_CONFIG_H

ROOT=$$LIBSRC/libvorbis

INCLUDEPATH += $$ROOT/include

lib_SOURCES = \
   mdct.c smallft.c block.c envelope.c window.c lsp.c \
   lpc.c analysis.c synthesis.c psy.c info.c \
   floor1.c floor0.c\
   res0.c mapping0.c registry.c codebook.c sharedbook.c\
   lookup.c bitrate.c

# add sources with full path
for (SRC, lib_SOURCES) {
   SOURCES += $$ROOT/lib/$$SRC
}

# message($$SOURCES)
addLibrary(ogg)|error(Failed to find ogg)

endProject()
