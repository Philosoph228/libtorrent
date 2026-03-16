#pragma once

#include <stdint.h>
#include <errno.h>

typedef uint64_t rlim_t;

struct rlimit {
  rlim_t rlim_cur;
  rlim_t rlim_max;
};

#ifndef RLIM_INFINITY
#define RLIM_INFINITY ((rlim_t)~0ULL)
#endif

#ifndef RLIMIT_AS
#define RLIMIT_AS 9
#endif

#ifndef RLIMIT_DATA
#define RLIMIT_DATA 2
#endif

static inline int getrlimit(int resource, struct rlimit* rlp) {
  (void)resource;
  (void)rlp;
  errno = ENOSYS;
  return -1;
}
