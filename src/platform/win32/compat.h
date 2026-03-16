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
#include <string>
#include <stdexcept>

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

// Convert a UTF-8 narrow string to a wide (UTF-16) string for Win32 APIs.
static inline std::wstring lt_utf8_to_wide(const char* s) {
  if (!s || !*s) return L"";
  int len = MultiByteToWideChar(CP_UTF8, 0, s, -1, nullptr, 0);
  if (len <= 0) return L"";
  std::wstring w(len - 1, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, s, -1, &w[0], len);
  return w;
}

static inline int lstat(const char* path, struct stat* st) {
  struct _stat64 st64;
  int r = _wstat64(lt_utf8_to_wide(path).c_str(), &st64);
  if (r == 0) {
    st->st_mode  = (unsigned short)st64.st_mode;
    st->st_size  = (long)st64.st_size;
    st->st_mtime = (time_t)st64.st_mtime;
    st->st_atime = (time_t)st64.st_atime;
    st->st_ctime = (time_t)st64.st_ctime;
    st->st_dev   = st64.st_dev;
    st->st_ino   = st64.st_ino;
    st->st_nlink = st64.st_nlink;
    st->st_uid   = st64.st_uid;
    st->st_gid   = st64.st_gid;
  }
  return r;
}

#ifndef mkdir
static inline int mkdir(const char* path, mode_t mode) {
  (void)mode;
  return _wmkdir(lt_utf8_to_wide(path).c_str());
}
#endif

// Intercept open() to use _wopen so UTF-8 paths work on Windows.
static inline int lt_open_utf8(const char* path, int flags, ...) {
  int mode = 0;
  if (flags & O_CREAT) {
    va_list ap;
    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);
  }
  return _wopen(lt_utf8_to_wide(path).c_str(), flags, mode);
}
// NOTE: Do NOT #define open here — it would break std::fstream::open() calls.
// Call lt_open_utf8() directly at POSIX open() call sites.

// Intercept stat() to use _wstat64 so UTF-8 paths work on Windows.
static inline int lt_stat_utf8(const char* path, struct stat* st) {
  struct _stat64 st64;
  int r = _wstat64(lt_utf8_to_wide(path).c_str(), &st64);
  if (r == 0) {
    st->st_mode  = (unsigned short)st64.st_mode;
    st->st_size  = (long)st64.st_size;
    st->st_mtime = (time_t)st64.st_mtime;
    st->st_atime = (time_t)st64.st_atime;
    st->st_ctime = (time_t)st64.st_ctime;
    st->st_dev   = st64.st_dev;
    st->st_ino   = st64.st_ino;
    st->st_nlink = st64.st_nlink;
    st->st_uid   = st64.st_uid;
    st->st_gid   = st64.st_gid;
  }
  return r;
}
// NOTE: Do NOT #define stat here — it would break local variables named 'stat'
// (e.g. utils::FileStat stat; stat.update(...)).
// Call lt_stat_utf8() directly at POSIX stat() call sites.

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

// unlink: delete a file by UTF-8 path
// On Windows, files created with O_RDONLY (0444) get the read-only attribute,
// which prevents deletion. Clear it first before unlinking.
static inline int unlink(const char* path) {
  std::wstring wpath = lt_utf8_to_wide(path);
  // Clear read-only attribute so we can delete it
  DWORD attrs = GetFileAttributesW(wpath.c_str());
  if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY))
    SetFileAttributesW(wpath.c_str(), attrs & ~FILE_ATTRIBUTE_READONLY);
  return _wunlink(wpath.c_str());
}

// rename: POSIX allows atomic replace of destination; Windows rename() does not.
// Use MoveFileExW with MOVEFILE_REPLACE_EXISTING instead.
// Call lt_rename_utf8() directly at call sites instead of using a macro,
// to avoid breaking std::rename().
static inline int lt_rename_utf8(const char* oldpath, const char* newpath) {
  std::wstring wsrc = lt_utf8_to_wide(oldpath);
  std::wstring wdst = lt_utf8_to_wide(newpath);

  // Try MoveFileExW with MOVEFILE_REPLACE_EXISTING first (atomic on same volume).
  if (MoveFileExW(wsrc.c_str(), wdst.c_str(),
                  MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH | MOVEFILE_COPY_ALLOWED))
    return 0;

  // If that failed (e.g. destination is open by antivirus/Defender),
  // try to delete the destination first, then retry the move.
  // Retry up to 5 times with 50ms delay to handle transient locks.
  DWORD dstAttrs = GetFileAttributesW(wdst.c_str());
  if (dstAttrs != INVALID_FILE_ATTRIBUTES) {
    if (dstAttrs & FILE_ATTRIBUTE_READONLY)
      SetFileAttributesW(wdst.c_str(), dstAttrs & ~FILE_ATTRIBUTE_READONLY);

    for (int i = 0; i < 5; i++) {
      if (DeleteFileW(wdst.c_str()))
        break;
      Sleep(50);
    }
  }

  if (MoveFileExW(wsrc.c_str(), wdst.c_str(),
                  MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH | MOVEFILE_COPY_ALLOWED))
    return 0;

  // Last resort: copy + delete source
  if (CopyFileW(wsrc.c_str(), wdst.c_str(), FALSE)) {
    DeleteFileW(wsrc.c_str());
    return 0;
  }

  // Map Win32 error to errno
  DWORD err = GetLastError();
  switch (err) {
    case ERROR_FILE_NOT_FOUND:
    case ERROR_PATH_NOT_FOUND: errno = ENOENT; break;
    case ERROR_ACCESS_DENIED:
    case ERROR_SHARING_VIOLATION: errno = EACCES; break;
    default: errno = EIO; break;
  }
  return -1;
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

// connect(): on Windows, non-blocking connect returns WSAEWOULDBLOCK instead of
// EINPROGRESS. Map it so POSIX code that checks errno == EINPROGRESS works.
static inline int lt_connect(SOCKET s, const struct sockaddr* name, int namelen) {
  int r = ::connect(s, name, namelen);
  if (r == SOCKET_ERROR) {
    int wsa_err = WSAGetLastError();
    if (wsa_err == WSAEWOULDBLOCK || wsa_err == WSAEINPROGRESS) {
      errno = EINPROGRESS;
    } else {
      switch (wsa_err) {
        case WSAECONNREFUSED: errno = ECONNREFUSED; break;
        case WSAENETUNREACH:  errno = ENETUNREACH;  break;
        case WSAETIMEDOUT:    errno = ETIMEDOUT;    break;
        case WSAENOBUFS:      errno = ENOBUFS;      break;
        default:              errno = wsa_err;      break;
      }
    }
    return -1;
  }
  return 0;
}
#define connect(s, name, namelen) lt_connect((SOCKET)(s), name, namelen)

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
