
include(../config.prf)

TEMPLATE=lib
#TARGET=flac
QT=
CONFIG += staticlib
DEFINES += HAVE_CONFIG_H

ROOT=$$LIBSRC/libflac

INCLUDEPATH += $$ROOT/include $$ROOT/src/libFLAC/include

libFLAC_sources = \
    metadata.cpp \
    stream_encoder.cpp \
    stream_decoder.cpp

# add sources with full path
#for (SRC, flac_SOURCES) {
#   SOURCES += $$ROOT/src/flac/$$SRC
#}

for (SRC, libFLAC_sources) {
   SOURCES += $$ROOT/src/libFLAC++/$$SRC
}

# message($$SOURCES)

     contains(ARCH,ia32):   CONFIG_H += FLAC__CPU_IA32,1
else:contains(ARCH,x86_64): CONFIG_H += FLAC__CPU_X86_64,1
else:contains(ARCH,armv7a): CONFIG_H += FLAC__CPU_ARM7A,1
else:error(Unsupported compiler or CPU)

cppDefines(__SSE__): CONFIG_H += FLAC__SSE_OS,1

contains(ARCH,ia32)|contains(ARCH,x86_64): CONFIG_H += FLAC__HAS_X86INTRIN

addLibrary(ogg) {
   CONFIG_H += FLAC__HAS_OGG,1
   for (SRC, extra_ogg_sources) {
      SOURCES += $$ROOT/src/libFLAC/$$SRC
   }
}
else:error(Failed to find ogg)

endProject()
