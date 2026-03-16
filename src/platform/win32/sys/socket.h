#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <winsock2.h>
#include <ws2tcpip.h>
#include <errno.h>
#include <string.h>

#ifndef socklen_t
typedef int socklen_t;
#endif

#ifndef sa_family_t
typedef ADDRESS_FAMILY sa_family_t;
#endif

#ifndef AF_LOCAL
#define AF_LOCAL AF_INET
#endif
#ifndef AF_UNIX
#define AF_UNIX AF_LOCAL
#endif

static inline int socketpair(int domain, int type, int protocol, int sv[2]) {
  if (sv == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (domain != AF_LOCAL && domain != AF_UNIX) {
    errno = WSAEAFNOSUPPORT;
    return -1;
  }

  if (type != SOCK_STREAM) {
    errno = WSAEPROTONOSUPPORT;
    return -1;
  }

  SOCKET listener = INVALID_SOCKET;
  SOCKET sock1 = INVALID_SOCKET;
  SOCKET sock2 = INVALID_SOCKET;

  struct sockaddr_in addr;
  int addrlen = sizeof(addr);

  listener = socket(AF_INET, SOCK_STREAM, protocol);
  if (listener == INVALID_SOCKET)
    goto fail;

  memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = 0;

  if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
    goto fail;

  if (listen(listener, 1) == SOCKET_ERROR)
    goto fail;

  if (getsockname(listener, (struct sockaddr*)&addr, &addrlen) == SOCKET_ERROR)
    goto fail;

  sock1 = socket(AF_INET, SOCK_STREAM, protocol);
  if (sock1 == INVALID_SOCKET)
    goto fail;

  if (connect(sock1, (struct sockaddr*)&addr, addrlen) == SOCKET_ERROR)
    goto fail;

  sock2 = accept(listener, NULL, NULL);
  if (sock2 == INVALID_SOCKET)
    goto fail;

  closesocket(listener);
  sv[0] = (int)sock1;
  sv[1] = (int)sock2;
  return 0;

fail:
  if (listener != INVALID_SOCKET)
    closesocket(listener);
  if (sock1 != INVALID_SOCKET)
    closesocket(sock1);
  if (sock2 != INVALID_SOCKET)
    closesocket(sock2);

  errno = WSAGetLastError();
  return -1;
}
