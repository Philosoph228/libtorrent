#pragma once

#include <BaseTsd.h>
#include <errno.h>
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <stdint.h>
#include <sys/types.h>
#include <windows.h>
#include <winsock2.h>

#ifndef ssize_t
typedef SSIZE_T ssize_t;
#endif

#ifndef STDIN_FILENO
#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2
#endif

#ifndef _SC_OPEN_MAX
#define _SC_OPEN_MAX 4
#endif

static inline int close(int fd) {
  if (fd == -1)
    return -1;

  int result = closesocket((SOCKET)fd);
  if (result == 0)
    return 0;

  int wsa_err = WSAGetLastError();
  if (wsa_err == WSAENOTSOCK || wsa_err == WSANOTINITIALISED)
    return _close(fd);

  errno = wsa_err;
  return -1;
}

static inline int pipe(int fds[2]) {
  return _pipe(fds, 512, O_BINARY);
}

// Map write -> _write to avoid ambiguity with CRT's int write(int,const void*,unsigned int)
#ifndef write
#define write _write
#endif

// Map read -> _read to avoid ambiguity with CRT's int read(int,void*,unsigned int)
#ifndef read
#define read _read
#endif

static inline int usleep(unsigned int usec) {
  Sleep((usec + 999) / 1000);
  return 0;
}

static inline unsigned int sleep(unsigned int sec) {
  Sleep(sec * 1000);
  return 0;
}

static inline long sysconf(int name) {
  if (name == _SC_OPEN_MAX)
    return 1024;

  errno = EINVAL;
  return -1;
}

static inline int getpagesize(void) {
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  return (int)si.dwPageSize;
}

static inline int ftruncate(int fd, int64_t size) {
  return _chsize_s(fd, size);
}
