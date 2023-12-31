#ifndef _SYSTEST_H_INCLUDED
#define _SYSTEST_H_INCLUDED

#if !defined(_WIN32)
# if defined(__APPLE__) && defined(__MACH__)
#  define __MACOS__
#  define _DARWIN_C_SOURCE
# elif defined(__FreeBSD__) || defined(__DragonFly__)
#  define __BSD__
#  define _BSD_SOURCE
# elif defined(__NetBSD__)
#  define __BSD__
#  define _NETBSD_SOURCE 1
# elif defined(__OpenBSD__)
#  define __BSD__
#  pragma message("don't know what to define for OpenBSD")
# elif defined(__linux__)
#  if !defined(_GNU_SOURCE)
#   define _GNU_SOURCE
#  endif
# else
#  define _DEFAULT_SOURCE
#  define _POSIX_C_SOURCE 200809L
#  define _XOPEN_SOURCE 700
# endif

#define __STDC_WANT_LIB_EXT1__ 1

#include <unistd.h>
#include <libgen.h>
#include <limits.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <sys/statvfs.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

# if defined(__GLIBC__)
#  if (__GLIBC__ >= 2 && __GLIBC_MINOR__ > 19)  || \
      (__GLIBC__ == 2 && __GLIBC_MINOR__ <= 19) && defined(_BSD_SOURCE)
#   define __HAVE_UNISTD_READLINK__
#  endif
# endif

# define BAD_SOCKET -1

# if defined(PATH_MAX)
#  define SYSTEST_MAXPATH PATH_MAX
# else
#  define SYSTEST_MAXPATH 1024
# endif
# define SYSTEST_MAXHOST 256
# define SYSTEST_PATH_SEP '/'

typedef int descriptor;
typedef socklen_t optlen;

#else /* __WIN__ */

# define __WIN__
# define __WANT_STDC_SECURE_LIB__ 1
# define WIN32_LEAN_AND_MEAN
# define WINVER       0x0A00
# define _WIN32_WINNT 0x0A00
# include <windows.h>
# include <shlwapi.h>
# include <direct.h>
# include <winsock2.h>
# include <ws2tcpip.h>
# include <wow64apiset.h>
# include <io.h>

# define BAD_SOCKET INVALID_SOCKET
# define SYSTEST_MAXPATH MAX_PATH
# define SYSTEST_MAXHOST 256
# define _SYS_NAMELEN 256
# define SYSTEST_PATH_SEP '\\'

struct utsname {
    char sysname[_SYS_NAMELEN];
    char nodename[_SYS_NAMELEN];
    char release[_SYS_NAMELEN];
    char version[_SYS_NAMELEN];
    char machine[_SYS_NAMELEN];
};

typedef SOCKET descriptor;
typedef int optlen;
#endif /* !__WIN__ */

#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>
#include <stdbool.h>
#include <assert.h>

#if !defined(__WIN__)
# if defined(__linux__)
#  include <sys/sysinfo.h>
#  define __HAVE_GET_NPROCS__
#  include <sched.h>
#  define __HAVE_SCHED__
# elif defined(__HAIKU__)
#  include <OS.h>
# else
#  include <sys/sysctl.h>
#  define __HAVE_SYSCTL__
# endif
#endif

#if defined(__MACOS__)
# include <mach-o/dyld.h>
#elif defined(__FreeBSD__)
# include <sys/sysctl.h>
#endif

#if defined(__clang__) && defined(__FILE_NAME__)
# define __file__ __FILE_NAME__
#elif defined(__GNUC__) && defined(__FILE_NAME__)
# define __file__ __FILE_NAME__
#else
# define __file__ (strrchr(__FILE__, SYSTEST_PATH_SEP) ? strrchr(__FILE__, SYSTEST_PATH_SEP) + 1 : __FILE__)
#endif

#ifdef __cplusplus
extern "C" {
#endif


//
// portability test definitions
//


///////////////////////////// file system //////////////////////////////////////

/** Defines how many characters to grow a buffer by which was deemed too small
 * by a system call (with no information regarding the necessary size). */
#define SYSTEST_PATH_BUFFER_GROW_BY 32

/** Buffer size, in characters, for struct stat -> string. */
#define SYSTEST_STAT_BUFFER_SIZE 128

/** Special flag to indicate to the caller that the file in question
 * does not exist (systest_pathgetstat). */
#define SYSTEST_STAT_NONEXISTENT ((off_t)0xffffff02)

/** Flags used to specify which directory to use as the base reference for
 * testing relative paths. */
typedef enum {
    SYSTEST_PATH_REL_TO_CWD = 0x0001,
    SYSTEST_PATH_REL_TO_APP = 0x0002
} systest_rel_to;

bool systest_pathgetstat(const char* restrict path, struct stat* restrict st, systest_rel_to rel_to);
bool systest_pathexists(const char* restrict path, bool* restrict exists, systest_rel_to rel_to);

char* systest_getcwd(void);

char* systest_getappfilename(void);
char* systest_getappbasename(void);
char* systest_getappdir(void);

char* systest_getbasename(char* restrict path);
char* systest_getdirname(char* restrict path);

bool systest_ispathrelative(const char* restrict path, bool* restrict relative);

char* systest_stattostring(struct stat* restrict st);

bool systest_add_slash(char* restrict path);

bool systest_getfreediskspace(uint64_t* bytes);

/////////////////////////////// network ////////////////////////////////////////

bool systest_haveinetconn(void);
bool systest_gethostname(char hname[SYSTEST_MAXHOST]);

/////////////////////////////// platform ///////////////////////////////////////

bool systest_getuname(struct utsname* name);
bool systest_getcpucount(int* ncpus);

//
// utility functions
//


/* this is strictly for use when encountering an actual failure of a system call.
 * use self_log to report things other than error numbers. */
void _handle_error(int err, const char* msg, char* file, int line, const char* func);
#define handle_error(err, msg) _handle_error(err, msg, __file__, __LINE__, __func__);

void _self_log(const char* msg, char* file, int line, const char* func);

static char _self_log_buf[512] = {0};
#define self_log(...)  \
    snprintf(_self_log_buf, 512, __VA_ARGS__); \
    _self_log(_self_log_buf, __file__, __LINE__, __func__);

static inline
void _systest_safefree(void** p) {
    if (!p || (p && !*p))
        return;
    free(*p);
    *p = NULL;
}


# define systest_safefree(pp) _systest_safefree((void**)pp)


static inline
void systest_safeclose(int* restrict fd) {
    if (!fd || (fd && 0 > *fd))
        return;

#if !defined(__WIN__)
    if (-1 == close(*fd))
#else
    if (-1 == _close(*fd))
#endif
        handle_error(errno, "close() failed!");

    *fd = -1;
}

/** Checks a bitmask for a specific set of bits. */
static inline
bool systest_bittest(uint32_t flags, uint32_t test) {
    return (flags & test) == test;
}

#if defined __countof
# undef __countof
#endif
#define __countof(arr) (sizeof((arr)) / sizeof((arr)[0]))

#ifdef __cplusplus
}
#endif

#endif // !_SYSTEST_H_INCLUDED
