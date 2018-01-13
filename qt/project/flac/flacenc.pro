
include(../config.pri)

TEMPLATE=app
#TARGET=flac
QT=
#CONFIG=staticlib debug
DEFINES += HAVE_CONFIG_H

ROOT=$$LIBSRC/libflac

INCLUDEPATH += $$ROOT/include $$ROOT/src/libFLAC/include

# pasted from src/flac/Makefile
flac_SOURCES = \
        analyze.c \
        decode.c \
        encode.c \
        foreign_metadata.c \
        main.c \
        local_string_utils.c \
        utils.c \
        vorbiscomment.c \

extra_ogg_sources = \
        ogg_decoder_aspect.c \
        ogg_encoder_aspect.c \
        ogg_helper.c \
        ogg_mapping.c

share_sources = \
        getopt/getopt.c \
        getopt/getopt1.c \
        grabbag/alloc.c \
        grabbag/cuesheet.c \
        grabbag/file.c \
        grabbag/picture.c \
        grabbag/replaygain.c \
        grabbag/seektable.c \
        grabbag/snprintf.c \
        replaygain_analysis/replaygain_analysis.c \
        replaygain_synthesis/replaygain_synthesis.c \
        utf8/charset.c \
        utf8/iconvert.c \
        utf8/utf8.c \

win32:share_sources += win_utf8_io/win_utf8_io.c
win32:DEFINES -= UNICODE

# add sources with full path
for (SRC, flac_SOURCES) {
   SOURCES += $$ROOT/src/flac/$$SRC
}

for (SRC, libFLAC_sources) {
   SOURCES += $$ROOT/src/libFLAC/$$SRC
}

for (SRC, share_sources) {
   SOURCES += $$ROOT/src/share/$$SRC
}

# message($$SOURCES)

     contains(ARCH,ia32):   CONFIG_H += FLAC__CPU_IA32,1
else:contains(ARCH,x86_64): CONFIG_H += FLAC__CPU_X86_64,1
else:contains(ARCH,armv7a): CONFIG_H += FLAC__CPU_ARM7A,1
else:error(Unsupported compiler or CPU)

contains(CPP, __SSE__): CONFIG_H += FLAC__SSE_OS,1

contains(ARCH,i686)|contains(ARCH,x86_64): CONFIG_H += FLAC__HAS_X86INTRIN

# force local libs to be used
CONFIG *= local_libs

addLibrary(ogg) {
   CONFIG_H += FLAC__HAS_OGG,1

   for (SRC, extra_ogg_sources) {
      SOURCES += $$ROOT/src/libFLAC/$$SRC
   }
}

!addLibrary(flac):error("Cannot find flac library")

endProject()
