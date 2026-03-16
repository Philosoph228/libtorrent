#pragma once

#include <windows.h>
#include <io.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>

#ifndef PROT_READ
#define PROT_READ 0x1
#endif
#ifndef PROT_WRITE
#define PROT_WRITE 0x2
#endif
#ifndef PROT_EXEC
#define PROT_EXEC 0x4
#endif
#ifndef PROT_NONE
#define PROT_NONE 0x0
#endif

#ifndef MAP_SHARED
#define MAP_SHARED 0x01
#endif
#ifndef MAP_PRIVATE
#define MAP_PRIVATE 0x02
#endif
#ifndef MAP_ANON
#define MAP_ANON 0x20
#endif
#ifndef MAP_ANONYMOUS
#define MAP_ANONYMOUS MAP_ANON
#endif

#ifndef MAP_FAILED
#define MAP_FAILED ((void*)-1)
#endif

#ifndef MS_SYNC
#define MS_SYNC 0x01
#endif
#ifndef MS_ASYNC
#define MS_ASYNC 0x02
#endif
#ifndef MS_INVALIDATE
#define MS_INVALIDATE 0x04
#endif

#ifndef MADV_NORMAL
#define MADV_NORMAL 0
#endif
#ifndef MADV_RANDOM
#define MADV_RANDOM 1
#endif
#ifndef MADV_SEQUENTIAL
#define MADV_SEQUENTIAL 2
#endif
#ifndef MADV_WILLNEED
#define MADV_WILLNEED 3
#endif
#ifndef MADV_DONTNEED
#define MADV_DONTNEED 4
#endif

static inline DWORD lt_mmap_protect(int prot) {
  if (prot & PROT_WRITE)
    return (prot & PROT_EXEC) ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
  if (prot & PROT_READ)
    return (prot & PROT_EXEC) ? PAGE_EXECUTE_READ : PAGE_READONLY;
  if (prot & PROT_EXEC)
    return PAGE_EXECUTE;
  return PAGE_NOACCESS;
}

static inline DWORD lt_mmap_access(int prot) {
  DWORD access = 0;
  if (prot & PROT_READ)
    access |= FILE_MAP_READ;
  if (prot & PROT_WRITE)
    access |= FILE_MAP_WRITE;
  if (prot & PROT_EXEC)
    access |= FILE_MAP_EXECUTE;
  if (access == 0)
    access = FILE_MAP_READ;
  return access;
}

static inline void* mmap(void* addr, size_t length, int prot, int flags, int fd, off_t offset) {
  (void)addr;

  if (length == 0) {
    errno = EINVAL;
    return MAP_FAILED;
  }

  HANDLE file_handle = INVALID_HANDLE_VALUE;
  if (!(flags & MAP_ANON)) {
    intptr_t os_handle = _get_osfhandle(fd);
    if (os_handle == -1) {
      errno = EBADF;
      return MAP_FAILED;
    }
    file_handle = (HANDLE)os_handle;
  }

  uint64_t max_size = (uint64_t)offset + (uint64_t)length;
  DWORD max_size_low = (DWORD)(max_size & 0xFFFFFFFFULL);
  DWORD max_size_high = (DWORD)((max_size >> 32) & 0xFFFFFFFFULL);

  HANDLE mapping = CreateFileMappingA(file_handle, NULL, lt_mmap_protect(prot), max_size_high, max_size_low, NULL);
  if (mapping == NULL) {
    errno = (int)GetLastError();
    return MAP_FAILED;
  }

  uint64_t off = (uint64_t)offset;
  void* view = MapViewOfFile(mapping, lt_mmap_access(prot), (DWORD)(off >> 32), (DWORD)(off & 0xFFFFFFFFULL), length);
  CloseHandle(mapping);

  if (view == NULL) {
    errno = (int)GetLastError();
    return MAP_FAILED;
  }

  return view;
}

static inline int munmap(void* addr, size_t length) {
  (void)length;
  if (addr == NULL || addr == MAP_FAILED) {
    errno = EINVAL;
    return -1;
  }

  if (!UnmapViewOfFile(addr)) {
    errno = (int)GetLastError();
    return -1;
  }

  return 0;
}

static inline int msync(void* addr, size_t length, int flags) {
  (void)flags;

  if (addr == NULL || addr == MAP_FAILED) {
    errno = EINVAL;
    return -1;
  }

  if (!FlushViewOfFile(addr, length)) {
    errno = (int)GetLastError();
    return -1;
  }

  return 0;
}

static inline int madvise(void* addr, size_t length, int advice) {
  (void)addr;
  (void)length;
  (void)advice;
  return 0;
}

static inline int mincore(void* addr, size_t length, unsigned char* vec) {
  (void)addr;

  if (vec == NULL) {
    errno = EINVAL;
    return -1;
  }

  memset(vec, 1, length ? (length + 4095) / 4096 : 0);
  return 0;
}
