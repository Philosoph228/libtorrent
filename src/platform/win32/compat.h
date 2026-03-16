#pragma once

#if defined(_WIN32)

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600
#endif

#include <BaseTsd.h>
#include <cstddef>
#include <cstdint>
#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <cerrno>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <direct.h>

#ifndef ssize_t
typedef SSIZE_T ssize_t;
#endif

#ifndef _LT_MODE_T_DEFINED
typedef unsigned short mode_t;
#define _LT_MODE_T_DEFINED
#endif

#ifndef _S_IFMT
#define _S_IFMT 0170000
#endif
#ifndef _S_IFREG
#define _S_IFREG 0100000
#endif
#ifndef _S_IFDIR
#define _S_IFDIR 0040000
#endif
#ifndef _S_IFCHR
#define _S_IFCHR 0020000
#endif
#ifndef _S_IFIFO
#define _S_IFIFO 0010000
#endif

#ifndef S_ISREG
#define S_ISREG(m) (((m) & _S_IFMT) == _S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(m) (((m) & _S_IFMT) == _S_IFDIR)
#endif
#ifndef S_ISCHR
#define S_ISCHR(m) (((m) & _S_IFMT) == _S_IFCHR)
#endif
#ifndef S_ISBLK
#define S_ISBLK(m) (0)
#endif
#ifndef S_ISFIFO
#define S_ISFIFO(m) (((m) & _S_IFMT) == _S_IFIFO)
#endif
#ifndef S_ISLNK
#define S_ISLNK(m) (0)
#endif
#ifndef S_ISSOCK
#define S_ISSOCK(m) (0)
#endif

static inline int lstat(const char* path, struct stat* st) {
  return stat(path, st);
}

#ifndef mkdir
static inline int mkdir(const char* path, mode_t mode) {
  (void)mode;
  return _mkdir(path);
}
#endif

#ifndef O_NONBLOCK
#define O_NONBLOCK 0x8000
#endif
#ifndef O_CLOEXEC
#define O_CLOEXEC 0
#endif
#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

#ifndef F_GETFL
#define F_GETFL 3
#endif
#ifndef F_SETFL
#define F_SETFL 4
#endif
#ifndef F_GETFD
#define F_GETFD 1
#endif
#ifndef F_SETFD
#define F_SETFD 2
#endif
#ifndef FD_CLOEXEC
#define FD_CLOEXEC 1
#endif

#ifdef _MSC_VER
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif
#endif

#ifndef __attribute__
#define __attribute__(x)
#endif

static inline long random(void) { return rand(); }
static inline void srandom(unsigned int seed) { srand(seed); }

static inline int lt_fcntl(int fd, int cmd, ...) {
  long arg = 0;

  if (cmd == F_SETFL || cmd == F_SETFD) {
    va_list ap;
    va_start(ap, cmd);
    arg = va_arg(ap, long);
    va_end(ap);
  }

  if (cmd == F_GETFL || cmd == F_GETFD)
    return 0;

  if (cmd == F_SETFD)
    return 0;

  if (cmd == F_SETFL) {
    if (arg & O_NONBLOCK) {
      u_long mode = 1;
      if (ioctlsocket((SOCKET)fd, FIONBIO, &mode) == 0)
        return 0;

      int wsa_err = WSAGetLastError();
      if (wsa_err != WSAENOTSOCK) {
        errno = wsa_err;
        return -1;
      }

      return 0;
    }

    return 0;
  }

  errno = EINVAL;
  return -1;
}

#ifndef fcntl
#define fcntl lt_fcntl
#endif

#endif // _WIN32
