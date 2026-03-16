#include "config.h"

#include "socket_datagram.h"

#include <sys/socket.h>

#include "torrent/exceptions.h"
#include "torrent/net/socket_address.h"

#ifdef _WIN32
#include <winsock2.h>
// Map WSAGetLastError() to errno so POSIX error-checking code works correctly.
static inline void wsa_to_errno() {
  int wsa_err = WSAGetLastError();
  switch (wsa_err) {
    case WSAEWOULDBLOCK:  errno = EAGAIN;       break;
    case WSAEINTR:        errno = EINTR;        break;
    case WSAECONNRESET:   errno = ECONNRESET;   break;
    case WSAECONNABORTED: errno = ECONNABORTED; break;
    case WSAEMSGSIZE:     errno = EMSGSIZE;     break;
    default:              errno = wsa_err;      break;
  }
}
#define MAP_WSA_ERRNO(r) do { if ((r) < 0) wsa_to_errno(); } while(0)
#else
#define MAP_WSA_ERRNO(r) (void)(r)
#endif

namespace torrent {

SocketDatagram::~SocketDatagram() = default;

int
SocketDatagram::read_datagram(void* buffer, unsigned int length) {
  if (length == 0)
    throw internal_error("Tried to receive buffer length 0");

  int r = ::recv(m_fileDesc, static_cast<char*>(buffer), length, 0);
  MAP_WSA_ERRNO(r);
  return r;
}

int
SocketDatagram::write_datagram(const void* buffer, unsigned int length) {
  if (length == 0)
    throw internal_error("Tried to send buffer length 0");

  int r = ::send(m_fileDesc, static_cast<const char*>(buffer), length, 0);
  MAP_WSA_ERRNO(r);
  return r;
}

int
SocketDatagram::read_datagram_sa(void* buffer, unsigned int length, sockaddr* from_sa, socklen_t from_length) {
  if (length == 0)
    throw internal_error("Tried to receive buffer length 0");

  if (from_sa == nullptr)
    throw internal_error("Tried to receive datagram with NULL sockaddr pointer");

  int r = ::recvfrom(m_fileDesc, static_cast<char*>(buffer), length, 0, from_sa, &from_length);
  MAP_WSA_ERRNO(r);
  return r;
}

int
SocketDatagram::write_datagram_sa(const void* buffer, unsigned int length, sockaddr* sa) {
  if (length == 0)
    throw internal_error("Tried to send buffer length 0");

  int r;

  if (sa != nullptr)
    r = ::sendto(m_fileDesc, static_cast<const char*>(buffer), length, 0, sa, sa_length(sa));
  else
    r = ::send(m_fileDesc, static_cast<const char*>(buffer), length, 0);

  MAP_WSA_ERRNO(r);
  return r;
}

} // namespace torrent
