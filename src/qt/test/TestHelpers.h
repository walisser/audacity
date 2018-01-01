

//
// MALLOC_SIZE
// return size of a pointer allocated through malloc()
//
#ifdef Q_OS_UNIX
#   include <malloc.h>
#   define MALLOC_SIZE(ptr) malloc_usable_size(ptr)
#else
#   error portme
#endif
