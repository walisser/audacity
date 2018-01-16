


//
// MALLOC_SIZE
// return size of a pointer allocated through malloc()
//
#ifdef Q_OS_UNIX
#   include <malloc.h>
#   define MALLOC_SIZE(ptr) malloc_usable_size(ptr)

#elif defined (Q_OS_WIN32) | defined(Q_OS_WIN64)
#   define MALLOC_SIZE(ptr) _msize(ptr)

#else
#   error portme
#endif

#ifdef Q_OS_UNIX
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
   bool apply(size_t bytes) { return false; }
};
#endif
