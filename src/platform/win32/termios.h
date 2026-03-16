#pragma once
// termios is not available on Windows; terminal control is via Win32 Console API
// Provide stub types/functions used by rtorrent display code
#include <winsock2.h>
#include <windows.h>

struct termios {
  unsigned int c_iflag;
  unsigned int c_oflag;
  unsigned int c_cflag;
  unsigned int c_lflag;
  unsigned char c_cc[20];
};

#define ECHO    0x0008
#define ICANON  0x0002
#define TCSANOW 0

static inline int tcgetattr(int fd, struct termios* t) { (void)fd; (void)t; return 0; }
static inline int tcsetattr(int fd, int action, const struct termios* t) { (void)fd; (void)action; (void)t; return 0; }
