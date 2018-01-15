include(../config.prf)

TEMPLATE=lib
#TARGET=expat
QT=
CONFIG += staticlib
DEFINES += HAVE_EXPAT_CONFIG_H

ROOT=$$LIBSRC/expat

INCLUDEPATH += $$ROOT/lib

expat_SOURCES = \
   xmlparse.c \
   xmlrole.c \
   xmltok_impl.c \
   xmltok_ns.c \
   xmltok.c

# add sources with full path
for (SRC, expat_SOURCES) {
   SOURCES += $$ROOT/lib/$$SRC
}

# message($$SOURCES)

endProject()
