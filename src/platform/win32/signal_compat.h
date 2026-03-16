#pragma once
// POSIX signal extensions for Windows/MSVC
// Provides sigaction, siginfo_t, sigemptyset, and missing signal numbers.

#if defined(_WIN32)
#include <signal.h>

// Missing signal numbers — map to unused values or define as no-ops
#ifndef SIGHUP
#define SIGHUP  1
#endif
#ifndef SIGQUIT
#define SIGQUIT 3
#endif
#ifndef SIGKILL
#define SIGKILL 9
#endif
#ifndef SIGPIPE
#define SIGPIPE 13
#endif
#ifndef SIGALRM
#define SIGALRM 14
#endif
#ifndef SIGBUS
#define SIGBUS  7
#endif

// siginfo_t stub
typedef struct {
  int   si_signo;
  int   si_code;
  int   si_errno;
  void* si_addr;   // fault address (for SIGBUS/SIGSEGV)
} siginfo_t;

// SA_* flags
#ifndef SA_RESTART
#define SA_RESTART  0x10000000
#endif
#ifndef SA_SIGINFO
#define SA_SIGINFO  0x00000004
#endif

// BUS error codes
#ifndef BUS_ADRALN
#define BUS_ADRALN 1
#define BUS_ADRERR 2
#define BUS_OBJERR 3
#endif

// SIGWINCH, SIGUSR1 — not real signals on Windows, use unused numbers
#ifndef SIGWINCH
#define SIGWINCH 28
#endif
#ifndef SIGUSR1
#define SIGUSR1  10
#endif

// Ensure NSIG is large enough to cover all signal numbers we define above.
// MSVC signal.h may define NSIG as 23, which is too small for SIGWINCH=28.
#ifndef NSIG
#define NSIG 32
#elif NSIG < 32
#undef NSIG
#define NSIG 32
#endif
typedef unsigned int sigset_t;
static inline int sigemptyset(sigset_t* set) { *set = 0; return 0; }
static inline int sigaddset(sigset_t* set, int sig) { (void)set; (void)sig; return 0; }
static inline int sigfillset(sigset_t* set) { *set = ~0u; return 0; }

// struct sigaction
struct sigaction {
  union {
    void (*sa_handler)(int);
    void (*sa_sigaction)(int, siginfo_t*, void*);
  };
  sigset_t sa_mask;
  int      sa_flags;
};

// sigaction(): on Windows just use signal() for basic handlers
static inline int sigaction(int signum, const struct sigaction* act, struct sigaction* oldact) {
  (void)oldact;
  if (act == nullptr) return 0;
  if (act->sa_flags & SA_SIGINFO) {
    // SA_SIGINFO handlers not supported on Windows — ignore
    return 0;
  }
  if (act->sa_handler == SIG_DFL || act->sa_handler == SIG_IGN) {
    signal(signum, act->sa_handler);
  } else {
    signal(signum, act->sa_handler);
  }
  return 0;
}

#endif // _WIN32
