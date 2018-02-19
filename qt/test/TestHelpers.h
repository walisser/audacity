#pragma once

#include "core/SampleFormat.h"

//
// MALLOC_SIZE
// return size of a pointer allocated through malloc()/new()
//
#if defined(Q_OS_MACOS)
#   include <malloc/malloc.h>
#   define MALLOC_SIZE(ptr) malloc_size(ptr)

#elif defined(Q_OS_UNIX)
#   include <malloc.h>
#   define MALLOC_SIZE(ptr) malloc_usable_size(ptr)

#elif defined(Q_OS_WIN32) | defined(Q_OS_WIN64)
#   define MALLOC_SIZE(ptr) _msize(ptr)

#else
#   error portme
#endif

// switch this on to help debugging
// disables FileSizeLimiter as it causes debugger stops on signal
//#define DEBUGGER_NEEDS_HELP 1
#ifdef DEBUGGER_NEEDS_HELP
#warning Some tests are disabled to aid debugging!
#endif

#if defined(Q_OS_UNIX) && !defined(DEBUGGER_NEEDS_HELP)
#include <sys/resource.h>
#include <signal.h>

// temporarily sets the maximum size in bytes of files that the process may create
// return false if the limit isn't supported
class FileSizeLimiter
{
public:
   FileSizeLimiter()
   {
      Q_REQUIRE( 0 == getrlimit(RLIMIT_FSIZE, &_limit) );
   }

   bool apply(size_t bytes)
   {
      _limit.rlim_cur = bytes;
      Q_REQUIRE( 0 == setrlimit(RLIMIT_FSIZE, &_limit) );

      // prevent signal on low disk, and return error from system call
      Q_REQUIRE( SIG_ERR != signal(SIGXFSZ, SIG_IGN) );

      return true;
   }

   // revert limits
   ~FileSizeLimiter()
   {
      _limit.rlim_cur = _limit.rlim_max;
      Q_REQUIRE( 0 == setrlimit(RLIMIT_FSIZE, &_limit) );
      Q_REQUIRE( SIG_ERR != signal(SIGXFSZ, SIG_DFL) );
   }
private:
   struct rlimit _limit;
};
#else
class FileSizeLimiter
{
public:
   bool apply(size_t bytes) { Q_UNUSED(bytes); return false; }
};
#endif


// test arrays for having all one byte value
static bool isValue(const void* ptr, size_t len, const uint8_t value)
{
   const uint8_t* b = (const uint8_t*)ptr;
   while (len--)
      if (*b++ != value)
         return false;

   return true;
}

static bool isZero(const void* ptr, size_t len) {
   return isValue(ptr, len, 0);
}

static bool isOne(const void* ptr, size_t len) {
   return isValue(ptr, len, 0xff);
}

static bool isEqual(const SampleBuffer& s1, const SampleBuffer& s2, sampleFormat fmt, size_t len)
{
    return memcmp(s1.ptr(), s2.ptr(), SAMPLE_SIZE(fmt)*len) == 0;
}

static bool isZero(const SampleBuffer& s1, sampleFormat fmt, int start, size_t len)
{
    return isZero((char*)s1.ptr() + start*SAMPLE_SIZE(fmt), SAMPLE_SIZE(fmt)*len);
}

static bool isOne(const SampleBuffer& s1, sampleFormat fmt, int start, size_t len)
{
    return isOne((char*)s1.ptr() + start*SAMPLE_SIZE(fmt), SAMPLE_SIZE(fmt)*len);
}

static void setByte(const SampleBuffer& s1, uint8_t value, sampleFormat fmt, int start, size_t len)
{
    memset((char*)s1.ptr()+ start*SAMPLE_SIZE(fmt), value, len*SAMPLE_SIZE(fmt));
}

static void setZero(const SampleBuffer& s1, sampleFormat fmt, int start, size_t len)
{
    setByte(s1, 0, fmt, start, len);
}

static void setOne(const SampleBuffer& s1, sampleFormat fmt, int start, size_t len)
{
    setByte(s1, 0xff, fmt, start, len);
}
