include(../config.prf)

TEMPLATE=lib
#TARGET=sndfile
QT=
CONFIG += staticlib

# work around for files with the same name
CONFIG += object_parallel_to_source

DEFINES += HAVE_CONFIG_H

ROOT=$$LIBSRC/libsndfile

INCLUDEPATH += $$ROOT/include

COMMON_SOURCES = \
   common.c file_io.c command.c pcm.c ulaw.c alaw.c float32.c \
   double64.c ima_adpcm.c ms_adpcm.c gsm610.c dwvw.c vox_adpcm.c \
   interleave.c strings.c dither.c broadcast.c audio_detect.c \
   ima_oki_adpcm.c ima_oki_adpcm.h chunk.c chanmap.c \
   windows.c id3.c \
   sndfile.c aiff.c au.c avr.c caf.c dwd.c flac.c g72x.c htk.c ircam.c \
   macbinary3.c macos.c mat4.c mat5.c nist.c ogg.c paf.c pvf.c raw.c rx2.c sd2.c \
   sds.c svx.c txw.c voc.c wve.c w64.c wav_w64.c wav.c xi.c mpc2k.c rf64.c \

G72X_SOURCES = \
   g72x.c g721.c g723_16.c g723_24.c g723_40.c

GSM610_SOURCES = \
   add.c decode.c gsm_decode.c gsm_encode.c long_term.c preprocess.c \
   short_term.c code.c gsm_create.c gsm_destroy.c gsm_option.c lpc.c rpe.c table.c

# add sources with full path
for (SRC, COMMON_SOURCES) {
   SOURCES += $$ROOT/src/$$SRC
}
for (SRC, G72X_SOURCES) {
   SOURCES += $$ROOT/src/G72x/$$SRC
}
for (SRC, GSM610_SOURCES) {
   SOURCES += $$ROOT/src/GSM610/$$SRC
}

# message($$SOURCES)
addLibrary(vorbis)|error(Failed to locate vorbis)
addLibrary(ogg)|error(Failed to locate ogg)
addLibrary(flac)|error(Failed to locate flac)

endProject()
