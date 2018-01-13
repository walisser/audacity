
#
# The general approach to project generation...
#
# - run ./configure
# - copy config.h into this directory
# - comment out stuff in config.h as needed and detect with qmake
# - write qmake_config.h and #include it from the modified config.h
#


###
# FUNCTION addLibrary
#
# configure a library from system with local sources as fallback
#
# ENVIRONMENT
# +++++++++++
# HOST       == ID of the host/target system/compiler/architecture
# LIBSRC     == absolute path to LOCAL sources (default:audacity/lib-src)
# PKG_CONFIG == pkg-config path (default:pkg-config)
# CONFIG(local_libs) == use local libs even if system lib is present in /usr/lib for example
# BUILD_DIR  == absolute path to build products (default:/.../qt/project) (must be absolute path)
#
# ARGS
# ++++
# $$1 = pkg-config package name e.g. xyz
#
# OUTPUT
# ++++++
# LIBXYZ_NAME    = libxyz
# LIBXYZ_VERSION = x.y.z
# LIBXYZ_SOURCE  = LOCAL|SYSTEM
#
###
defineTest(addLibrary) {

   name=$$1
   libname=lib$$name
   prefix=$$upper($$libname)
   source=INVALID
   version=0.0.0
   srcdir=$$LIBSRC/$$libname

   #message(PKG_CONFIG=$$PKG_CONFIG)
   #message(LIBSRC=$$LIBSRC)

   local_libs|!system("$$PKG_CONFIG $$name --exists") {

      !exists($$srcdir/configure.ac) {
         warning(configLibrary $$name: expected sources in $$absolute_path($$srcdir))
         return(false)
      }

      INCLUDEPATH *= $$srcdir/include
      INCLUDEPATH *= $$BUILD_DIR/$$name
      LIBS        *= $$BUILD_DIR/$$name/build/$$HOST/$${libname}.a

      export(INCLUDEPATH)
      export(LIBS)
      export(CONFIG)

      source = LOCAL
      version = $$system("cat $$srcdir/configure.ac | grep AC_INIT | sed -r 's/.+\\[([0-9\\.]+)\\].+/\\1/'")
   }
   else: {

      flags = $$system("$$PKG_CONFIG $$name --cflags")

      QMAKE_CFLAGS *= $$flags
      QMAKE_CXXFLAGS *= $$flags
      LIBS *= $$system("$$PKG_CONFIG $$name --libs")

      export(QMAKE_CFLAGS)
      export(QMAKE_CXXFLAGS)

      source = SYSTEM
      version = $$system("$$PKG_CONFIG $$name --modversion")
    }

   !staticlib {
      export(LIBS)
   }

   message(Using $$source $$libname v$$version)

   $${prefix}_NAME    = $$libname
   $${prefix}_SOURCE  = $$source
   $${prefix}_VERSION = $$version

   export($${prefix}_NAME)
   export($${prefix}_SOURCE)
   export($${prefix}_VERSION)

   return(true)
}

###
# FUNCTION test
#
# Test a feature by attempting to compile a header.
# We can't execute anything because we must be able
# to cross-compile.
#
# -Compiles a file [feature].h containing static assertion
# -If success, includes [feature].pri, which add defines to CONFIG_H
# -Caches the result in BUILD_DIR/cache/.../[feature]-[yes|no]
# -Contents of CONFIG_H eventually end up in qmake_config.h
##
defineTest(test) {

   feature=$$1
   src=./$${feature}.h
   inc=./$${feature}.pri
   cachefile=$$BUILD_DIR/cache/$$HOST/$$feature # FIXME should be PROJECT_DIR
   cachedir=$$dirname(cachefile)
   ok=false
   cached=''

   mkpath($$cachedir)|error("failed to make cache dir (1)")
   exists($$cachedir)|error("failed to make cache dir (2)")

   !exists($$src):error(no source file for test [$$feature])

   exists($$cachefile-yes) {
      include($$inc)
      export(CONFIG_H)
      ok=true
      cached=" (cached)"
   }
   else:exists($$cachefile-no) {
      cached=" (cached)"
   }
   else {

      system("$$QMAKE_CXX $$QMAKE_CXXFLAGS -std=c++11 -x c++ -fsyntax-only  $$src >> '$${CONFIG_LOG}' 2>&1") {
         include($$inc)
         export(CONFIG_H)
         write_file($${cachefile}-yes, feature)|error("failed to cache result (y)")
         ok=true
      }
      else {
         write_file($$cachefile-no, feature)|error("failed to cache result(n)")
      }
   }

   if ($$ok):message([$$feature]=yes$${cached})
   else:message([$$feature]=no$${cached})

   return($$ok)
}

###
# FUNCTION fail
#
# Fail a test and provided a useful message.
#
###
defineTest(fail) {
   log=$$relative_path($$CONFIG_LOG,$$_PRO_FILE_PWD_)
   error("Configuration test [$$1] failed, details in ./$$log")
   return(false)
}

###
# FUNCTION endProject()
#
# Common finishing stuff, must be last thing in the .pro
# Writes CONFIG_H contents to qmake_config.h
#
###
defineTest(endProject) {

   # clean up after lib detection
   QMAKE_CFLAGS=$$unique(QMAKE_CFLAGS)
   QMAKE_CXXFLAGS=$$unique(QMAKE_CFLAGS)
   LIBS=$$unique(LIBS)

   message(INCLUDEPATH=$$INCLUDEPATH)
   write_file($$CONFIG_LOG,INCLUDEPATH,append)

   message(LIBS=$$LIBS)
   write_file($$CONFIG_LOG,LIBS,append)

   message(CFLAGS=$$QMAKE_CFLAGS)
   write_file($$CONFIG_LOG,CONFIG_H,append)

   message(CXXFLAGS=$$QMAKE_CXXFLAGS)
   write_file($$CONFIG_LOG,QMAKE_CFLAGS,append)


   # do the foo for ./configure emulation
   D='$$escape_expand(\\n)$${LITERAL_HASH}define '
   CONFIG_H=$$join(CONFIG_H, $$D, $$D)
   CONFIG_H=$$replace(CONFIG_H, ",", " ")

   message(CONFIG_H=$$CONFIG_H)
   message(CONFIG=$$CONFIG)

   write_file($$CONFIG_LOG,CONFIG_H,append)
   write_file($$CONFIG_LOG,CONFIG,append)

   # FIXME: add to clean files
   OUTFILE=$$DESTDIR/qmake_config.h
   write_file($$OUTFILE, CONFIG_H): {

      message(wrote $$OUTFILE)
   }

   # diagnostic, show all qmake variables
   #for(var, $$list($$enumerate_vars())) {
   #    message(VAR=$$var)
   #    message("   "$$eval($$var))
   #}
}

# common setup
PKG_CONFIG=pkg-config
LIBSRC=../../../lib-src
BUILD_DIR=$$(HOME)/sw/audacity/qt/project

# config options
#CONFIG += prefer_local_libs

QT=

# nuke default stuff
CONFIG -= debug_and_release
CONFIG -= debug_and_release_target
CONFIG -= release
CONFIG *= debug
#CONFIG *= release

win32 {
   CONFIG *= console
   QMAKE_CFLAGS   *= -march=core2
   QMAKE_CXXFLAGS *= -march=core2
}

#
# ./configure emulation...
# - stuff configure outputs in CONFIG_H variables,
# - values cannot contain spaces, use COMMA for space
# - endProject() writes them to build/$$HOST/qmake_config.h
# - qmake_config.h is included from generic config.h
#
#CONFIG_H +=

# ensure the config.h that is pulled in comes from the project directory,
# and not some remnants from the source directory
INCLUDEPATH = $$_PRO_FILE_PWD_

# we're always going to use local libs on some targets
android|win32|macx|ios {
   CONFIG *= local_libs
}

# Get all preprocessor defines from compiler and use to detect arch
# This way we do not have to compile any tests for most things
# TODO: this only works on gcc & clang on unixen
CPP=$$system("$$QMAKE_CC $$QMAKE_CFLAGS -dM -E - </dev/null")
message($$CPP)

ARCH=unknown

contains(CPP, __i386__)|contains(CPP, __x86_64__) {

   message(x86 architecture detected)

   # x64 test first since __core2__ could be 32 or 64-bit
        contains(CPP, __x86_64__): ARCH = x86_64
   else:contains(CPP, __i686__):   ARCH = ia32
   else:contains(CPP, __core2__):  ARCH = ia32
   else:error(Unknown x86 architecture)

   CONFIG_H += CPU_IS_BIG_ENDIAN,0
   CONFIG_H += CPU_IS_LITTLE_ENDIAN,1
   CONFIG_H += WORDS_BIGENDIAN,0
   CONFIG_H += HAVE_X86INTRIN_H
}
else:contains(CPP, __ARM_ARCH_7A__) {

   message(armv7a architecture detected)
   ARCH = armv7a

   # TODO arm could be big or little...

   contains(CPP, ANDROID) {
      CONFIG_H += CPU_IS_BIG_ENDIAN,0
      CONFIG_H += CPU_IS_LITTLE_ENDIAN,1
      CONFIG_H += WORDS_BIGENDIAN,0
   }
}
else {
   error(Unsupported compiler or CPU)
}

# we know arch now so setup build paths
HOST = $$basename(QMAKESPEC)-$$ARCH

# mingw-ar chokes if "++" is in a path...
HOST = $$replace(HOST, "g\+\+", "gcc")

# TODO: set DESTDIR optionally for out-of-tree builds
DESTDIR      = $$_PRO_FILE_PWD_/build/$$HOST
OBJECTS_DIR  = $$DESTDIR
MOC_DIR      = $$DESTDIR
INCLUDEPATH += $$DESTDIR
CONFIG_LOG   = $$absolute_path($$DESTDIR/config.log)

message($$ARCH)
message(DESTDIR=$$DESTDIR)
message(LOGFILE=$$CONFIG_LOG)

mkpath($$DESTDIR)|error(Failed to create build directory)


# reset config log and add some useful info
write_file($$CONFIG_LOG, _DATE_)
write_file($$CONFIG_LOG, HOST)

# do some configure tests and output autoconf-style HAVE_XXX and friends
message(Testing compiler...)

# tests generated by config.tests/sizeof.py
include(./config/sizeof.pri)|error(Missing sizeof tests... run sizeof.py!)

#cache()
#cache(THING)|cache(THING, set, HOST)
#message(cacheed..$$THING)


!android:!win32 {
   CONFIG_H += HAVE_ICONV,1
   CONFIG_H += HAVE_LANGINFO_CODESET,1
}

!win32 {
   CONFIG_H += HAVE_BYTESWAP,1
   CONFIG_H += HAVE_GMTIME_R,1
   CONFIG_H += HAVE_ALLOCA_H,1
}
