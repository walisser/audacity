include(../config.pri)

TEMPLATE=lib
#TARGET=ogg
QT=
CONFIG += staticlib
DEFINES += HAVE_CONFIG_H

ROOT=$$LIBSRC/libogg

INCLUDEPATH += $$ROOT/include

ogg_SOURCES = \
   bitwise.c \
   framing.c

# add sources with full path
for (SRC, ogg_SOURCES) {
   SOURCES += $$ROOT/src/$$SRC
}

# message($$SOURCES)

endProject()
