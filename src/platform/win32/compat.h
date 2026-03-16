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

// MSVC defines off_t as 'long' (32-bit). Override it to int64_t so that
// libtorrent's sizeof(off_t) != 8 check passes and large files work correctly.
// Must be done before any header that pulls in <sys/types.h>.
#ifdef _MSC_VER
#  ifdef _off_t
#    undef _off_t
#  endif
#  ifdef off_t
#    undef off_t
#  endif
typedef int64_t off_t;
#  define _off_t off_t
#  define _OFF_T_DEFINED
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <direct.h>

// Auto-initialize Winsock on Windows so callers don't need to call WSAStartup.
// This runs before main() via a static constructor.
namespace {
struct WinsockInit {
  WinsockInit() {
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
  }
  ~WinsockInit() {
    WSACleanup();
  }
} _winsock_init;
} // namespace

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

// access() flags
#ifndef F_OK
#define F_OK 0
#define R_OK 4
#define W_OK 2
#define X_OK 1
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

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

// fdatasync: use _commit() on Windows (flushes file buffers)
static inline int fdatasync(int fd) {
  return _commit(fd);
}

// symlink: requires developer mode or admin on Windows
static inline int symlink(const char* target, const char* linkpath) {
  if (CreateSymbolicLinkA(linkpath, target, 0))
    return 0;
  errno = EPERM;
  return -1;
}

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

// POSIX signal extensions (sigaction, siginfo_t, etc.)
#include "signal_compat.h"

// PDCursesMod uses resize_term instead of resizeterm
#ifndef resizeterm
#define resizeterm resize_term
#endif

// set_escdelay: ncurses extension not in PDCursesMod; no-op on Windows
#ifndef set_escdelay
static inline int set_escdelay(int) { return 0; }
#endif

// MSVC does not have __builtin_popcount; use intrinsic instead
#ifdef _MSC_VER
#include <intrin.h>
#define __builtin_popcount(x)   (int)__popcnt((unsigned int)(x))
#define __builtin_popcountl(x)  (int)__popcnt((unsigned long)(x))
#define __builtin_popcountll(x) (int)__popcnt64((unsigned long long)(x))
#define USE_BUILTIN_POPCOUNT 1

// POSIX wide-char display width functions
#include <cwchar>
static inline int wcwidth(wchar_t c) {
  if (c == 0) return 0;
  return 1;
}
static inline int wcswidth(const wchar_t* s, size_t n) {
  int w = 0;
  for (size_t i = 0; i < n && s[i]; ++i) w += wcwidth(s[i]);
  return w;
}

// POSIX localtime_r -> MSVC localtime_s (reversed argument order)
#include <ctime>
static inline struct tm* localtime_r(const time_t* timep, struct tm* result) {
  return (localtime_s(result, timep) == 0) ? result : nullptr;
}
static inline struct tm* gmtime_r(const time_t* timep, struct tm* result) {
  return (gmtime_s(result, timep) == 0) ? result : nullptr;
}

// POSIX process functions via Win32
#include <process.h>
#include <io.h>

// dup2
static inline int dup2(int oldfd, int newfd) {
  return _dup2(oldfd, newfd);
}

// getpid
static inline int getpid(void) {
  return (int)GetCurrentProcessId();
}

// getppid: no parent process concept on Windows, return 0
static inline int getppid(void) {
  return 0;
}

// srand48/drand48: not in MSVC CRT
static inline void srand48(long seed) { srand((unsigned int)seed); }
static inline double drand48(void) { return (double)rand() / ((double)RAND_MAX + 1.0); }
static inline long lrand48(void) { return rand(); }

// pid_t must be defined before kill() and fork()
#ifndef _LT_PID_T_DEFINED
typedef DWORD pid_t;
#define _LT_PID_T_DEFINED
#endif

// kill: only signal 0 (existence check) is supported on Windows
static inline int kill(pid_t pid, int sig) {
  if (sig == 0) {
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, (DWORD)pid);
    if (h == NULL) { errno = ESRCH; return -1; }
    DWORD code = 0;
    GetExitCodeProcess(h, &code);
    CloseHandle(h);
    if (code != STILL_ACTIVE) { errno = ESRCH; return -1; }
    return 0;
  }
  errno = EINVAL;
  return -1;
}

// fork/execvp: implement via _spawnvp
// Since Windows has no fork, exec_file.cc's fork()+execvp() pattern is
// handled by making fork() return 0 (child branch) and execvp() spawn+exit.

static inline pid_t fork() {
  return 0;
}

static inline int execvp(const char* file, char* const argv[]) {
  intptr_t ret = _spawnvp(_P_WAIT, file, (const char* const*)argv);
  ExitProcess((UINT)ret);
  return -1;
}

#ifndef _exit
static inline void _exit(int code) {
  ExitProcess((UINT)code);
}
#endif

// strsignal: not in MSVC CRT
static inline const char* strsignal(int sig) {
  switch (sig) {
  case SIGINT:  return "Interrupt";
  case SIGTERM: return "Terminated";
  case SIGABRT: return "Aborted";
  case SIGFPE:  return "Floating point exception";
  case SIGILL:  return "Illegal instruction";
  case SIGSEGV: return "Segmentation fault";
  default:      return "Unknown signal";
  }
}

#endif // _MSC_VER

#endif // _WIN32
