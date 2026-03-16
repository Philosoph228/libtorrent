#pragma once

#include <sys/socket.h>

#ifndef sa_family_t
typedef ADDRESS_FAMILY sa_family_t;
#endif

#ifndef _WIN32_SOCKADDR_UN_DEFINED
#define _WIN32_SOCKADDR_UN_DEFINED
struct sockaddr_un {
  sa_family_t sun_family;
  char        sun_path[108];
};
#endif
