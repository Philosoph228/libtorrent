#pragma once

#include <windows.h>
#include <process.h>
#include <stdint.h>
#include <errno.h>

#ifdef __cplusplus
#include <mutex>
#include <unordered_map>
extern "C" {
#endif

typedef uintptr_t pthread_t;
typedef void* (*pthread_start_routine)(void*);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
namespace {
struct lt_pthread_start_data {
  pthread_start_routine fn;
  void* arg;
};

static inline std::unordered_map<pthread_t, HANDLE>& lt_pthread_handle_map() {
  static std::unordered_map<pthread_t, HANDLE> handles;
  return handles;
}

static inline std::mutex& lt_pthread_handle_mutex() {
  static std::mutex m;
  return m;
}

static unsigned __stdcall lt_pthread_start_thunk(void* raw) {
  auto* data = static_cast<lt_pthread_start_data*>(raw);
  if (data == nullptr)
    return 0;

  try {
    data->fn(data->arg);
  } catch (const std::exception& e) {
    // Exceptions must not propagate out of a Windows thread function.
    // Log to stderr so the user can see what went wrong.
    fprintf(stderr, "lt_pthread_start_thunk: uncaught exception: %s\n", e.what());
    fflush(stderr);
    // Terminate the process cleanly rather than crashing via std::terminate.
    ExitProcess(1);
  } catch (...) {
    fprintf(stderr, "lt_pthread_start_thunk: uncaught unknown exception\n");
    fflush(stderr);
    ExitProcess(1);
  }

  delete data;
  return 0;
}
} // namespace

extern "C" {
static inline int pthread_create(pthread_t* thread, const void* attr, pthread_start_routine start_routine, void* arg) {
  (void)attr;

  if (thread == nullptr || start_routine == nullptr)
    return EINVAL;

  auto* data = new lt_pthread_start_data{start_routine, arg};

  unsigned int thread_id = 0;
  HANDLE handle = (HANDLE)_beginthreadex(nullptr, 0, lt_pthread_start_thunk, data, 0, &thread_id);

  if (handle == 0) {
    delete data;
    return EAGAIN;
  }

  {
    auto lock = std::lock_guard(lt_pthread_handle_mutex());
    lt_pthread_handle_map()[static_cast<pthread_t>(thread_id)] = handle;
  }

  *thread = static_cast<pthread_t>(thread_id);
  return 0;
}

static inline int pthread_join(pthread_t thread, void** retval) {
  if (retval != nullptr)
    *retval = nullptr;

  HANDLE handle = NULL;
  {
    auto lock = std::lock_guard(lt_pthread_handle_mutex());
    auto& map = lt_pthread_handle_map();
    auto itr = map.find(thread);
    if (itr != map.end()) {
      handle = itr->second;
      map.erase(itr);
    }
  }

  if (handle == NULL)
    handle = OpenThread(SYNCHRONIZE, FALSE, static_cast<DWORD>(thread));

  if (handle == NULL)
    return ESRCH;

  WaitForSingleObject(handle, INFINITE);
  CloseHandle(handle);
  return 0;
}

static inline pthread_t pthread_self(void) {
  return static_cast<pthread_t>(GetCurrentThreadId());
}

static inline int pthread_setname_np(pthread_t thread, const char* name) {
  (void)thread;
  (void)name;
  return 0;
}
}

static inline int pthread_setname_np(const char* name) {
  return pthread_setname_np(pthread_self(), name);
}
#endif
