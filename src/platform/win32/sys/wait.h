#pragma once
// sys/wait.h is not available on Windows; process waiting uses Win32 API
#include <windows.h>
typedef DWORD pid_t;
#define WIFEXITED(s)   (1)
#define WEXITSTATUS(s) ((s) & 0xff)
#define WIFSIGNALED(s) (0)
#define WTERMSIG(s)    (0)

static inline pid_t waitpid(pid_t pid, int* status, int options) {
  HANDLE h = OpenProcess(SYNCHRONIZE | PROCESS_QUERY_INFORMATION, FALSE, pid);
  if (!h) { errno = ECHILD; return -1; }
  WaitForSingleObject(h, INFINITE);
  DWORD code = 0;
  GetExitCodeProcess(h, &code);
  CloseHandle(h);
  if (status) *status = (int)code;
  return pid;
}
