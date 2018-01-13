TEMPLATE=subdirs

CONFIG *= local_libs

SUBDIRS= \
   ogg \
   vorbis \
   vorbisenc \
   vorbisfile \
   flac \
   sndfile \
   flacenc

vorbisenc.file = vorbis/vorbisenc.pro
vorbisfile.file = vorbis/vorbisfile.pro

flacenc.file = flac/flacenc.pro

vorbis.depends = ogg
vorbisfile.depends = ogg
vorbisenc.depends = ogg
flacenc.depends = flac ogg
sndfile.depends = ogg vorbis vorbisenc vorbisfile flac
