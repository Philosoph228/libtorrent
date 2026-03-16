#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <errno.h>

#ifndef inet_ntop
static inline const char* lt_inet_ntop(int af, const void* src, char* dst, socklen_t size) {
  const char* result = InetNtopA(af, (PVOID)src, dst, size);
  if (result == NULL)
    errno = WSAGetLastError();
  return result;
}
#define inet_ntop lt_inet_ntop
#endif

#ifndef inet_pton
static inline int lt_inet_pton(int af, const char* src, void* dst) {
  int result = InetPtonA(af, src, dst);
  if (result == -1)
    errno = WSAGetLastError();
  return result;
}
#define inet_pton lt_inet_pton
#endif
