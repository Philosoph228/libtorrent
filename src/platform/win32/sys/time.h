#pragma once

#include <winsock2.h>
#include <windows.h>
#include <stdint.h>
#include <errno.h>

#ifndef _TIMEVAL_DEFINED
#define _TIMEVAL_DEFINED
struct timeval {
  long tv_sec;
  long tv_usec;
};
#endif

static inline int gettimeofday(struct timeval* tv, void* tz) {
  (void)tz;
  if (tv == NULL) {
    errno = EINVAL;
    return -1;
  }

  FILETIME ft;
  GetSystemTimeAsFileTime(&ft);

  ULARGE_INTEGER uli;
  uli.LowPart = ft.dwLowDateTime;
  uli.HighPart = ft.dwHighDateTime;

  const uint64_t epoch_diff = 116444736000000000ULL;
  uint64_t time = uli.QuadPart - epoch_diff;

  tv->tv_sec = (long)(time / 10000000ULL);
  tv->tv_usec = (long)((time % 10000000ULL) / 10ULL);
  return 0;
}
