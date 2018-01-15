
include(../config.prf)

TEMPLATE=lib
#TARGET=flac
QT=
CONFIG += staticlib
DEFINES += HAVE_CONFIG_H

ROOT=$$LIBSRC/libflac

INCLUDEPATH += $$ROOT/include $$ROOT/src/libFLAC/include

libFLAC_sources = \
        bitmath.c \
        bitreader.c \
        bitwriter.c \
        cpu.c \
        crc.c \
        fixed.c \
        fixed_intrin_sse2.c \
        fixed_intrin_ssse3.c \
        float.c \
        format.c \
        lpc.c \
        lpc_intrin_sse.c \
        lpc_intrin_sse2.c \
        lpc_intrin_sse41.c \
        lpc_intrin_avx2.c \
        md5.c \
        memory.c \
        metadata_iterators.c \
        metadata_object.c \
        stream_decoder.c \
        stream_encoder.c \
        stream_encoder_intrin_sse2.c \
        stream_encoder_intrin_ssse3.c \
        stream_encoder_intrin_avx2.c \
        stream_encoder_framing.c \
        window.c

extra_ogg_sources = \
        ogg_decoder_aspect.c \
        ogg_encoder_aspect.c \
        ogg_helper.c \
        ogg_mapping.c

win32:share_sources += win_utf8_io/win_utf8_io.c
win32:DEFINES -= UNICODE

# add sources with full path
#for (SRC, flac_SOURCES) {
#   SOURCES += $$ROOT/src/flac/$$SRC
#}

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
