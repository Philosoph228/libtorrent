#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdint.h>

#ifndef in_addr_t
typedef uint32_t in_addr_t;
#endif

#ifndef socklen_t
typedef int socklen_t;
#endif

#ifndef INET_ADDRSTRLEN
#define INET_ADDRSTRLEN 16
#endif

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 46
#endif
