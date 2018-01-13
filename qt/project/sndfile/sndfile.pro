include(../config.pri)

TEMPLATE=lib
#TARGET=sndfile
QT=
CONFIG += staticlib
DEFINES += HAVE_CONFIG_H

ROOT=$$LIBSRC/libsndfile

INCLUDEPATH += $$ROOT/include

COMMON_SOURCES = \
   common.c file_io.c command.c pcm.c ulaw.c alaw.c float32.c \
   double64.c ima_adpcm.c ms_adpcm.c gsm610.c dwvw.c vox_adpcm.c \
   interleave.c strings.c dither.c broadcast.c audio_detect.c \
   ima_oki_adpcm.c ima_oki_adpcm.h chunk.c chanmap.c \
   windows.c id3.c ogg.c flac.c \

   #$(WIN_VERSION_FILE)


# add sources with full path
for (SRC, COMMON_SOURCES) {
   SOURCES += $$ROOT/src/$$SRC
}

# message($$SOURCES)
addLibrary(vorbis)|error(Failed to locate vorbis)
addLibrary(ogg)|error(Failed to locate ogg)
addLibrary(flac)|error(Failed to locate flac)

endProject()
