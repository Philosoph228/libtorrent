#pragma once
// On Windows, ioctl equivalents are in winsock2.h (ioctlsocket)
// Provide TIOCGWINSZ / winsize stub that uses Win32 Console API
#include <winsock2.h>
#include <windows.h>

struct winsize {
  unsigned short ws_row;
  unsigned short ws_col;
  unsigned short ws_xpixel;
  unsigned short ws_ypixel;
};

#define TIOCGWINSZ 0x5413

// Use a typed overload instead of va_list to avoid MSVC stack corruption.
static inline int ioctl(int /*fd*/, unsigned long request, struct winsize* ws) {
  if (request == TIOCGWINSZ && ws != nullptr) {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
      ws->ws_col    = (unsigned short)(csbi.srWindow.Right  - csbi.srWindow.Left + 1);
      ws->ws_row    = (unsigned short)(csbi.srWindow.Bottom - csbi.srWindow.Top  + 1);
      ws->ws_xpixel = 0;
      ws->ws_ypixel = 0;
      return 0;
    }
    return -1;
  }
  return -1;
}
