#pragma once
// POSIX dirent.h emulation for Windows
#include <windows.h>
#include <cstring>
#include <cerrno>

#define DT_UNKNOWN 0
#define DT_REG     8
#define DT_DIR     4
#define DT_LNK     10

struct dirent {
  unsigned long d_fileno;   // inode number (not meaningful on Windows, set to 0)
  unsigned short d_reclen;  // record length (not meaningful on Windows, set to 0)
  unsigned char d_type;
  char          d_name[MAX_PATH];
};

struct DIR {
  HANDLE          handle;
  WIN32_FIND_DATAA data;
  bool            first;
  struct dirent   entry;
};

static inline DIR* opendir(const char* path) {
  char pattern[MAX_PATH];
  snprintf(pattern, sizeof(pattern), "%s\\*", path);
  DIR* d = new DIR{};
  d->handle = FindFirstFileA(pattern, &d->data);
  if (d->handle == INVALID_HANDLE_VALUE) { delete d; errno = ENOENT; return nullptr; }
  d->first = true;
  return d;
}

static inline struct dirent* readdir(DIR* d) {
  if (d->first) { d->first = false; }
  else if (!FindNextFileA(d->handle, &d->data)) return nullptr;
  strncpy(d->entry.d_name, d->data.cFileName, MAX_PATH - 1);
  d->entry.d_name[MAX_PATH - 1] = '\0';
  d->entry.d_fileno = 0;
  d->entry.d_reclen = 0;
  d->entry.d_type = (d->data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? DT_DIR : DT_REG;
  return &d->entry;
}

static inline int closedir(DIR* d) {
  FindClose(d->handle);
  delete d;
  return 0;
}
