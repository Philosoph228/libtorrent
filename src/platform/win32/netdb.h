#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#ifndef gai_strerror
#define gai_strerror gai_strerrorA
#endif

#ifndef EAI_SYSTEM
#define EAI_SYSTEM WSASYSCALLFAILURE
#endif

#ifndef EAI_ADDRFAMILY
#define EAI_ADDRFAMILY EAI_NONAME
#endif
