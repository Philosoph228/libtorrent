#include "config.h"

#if defined(WINDOWS)

#include "torrent/net/poll.h"

#include <cassert>
#include <cerrno>
#include <cstdint>
#include <map>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>

#include "torrent/event.h"
#include "torrent/exceptions.h"
#include "torrent/utils/log.h"
#include "torrent/utils/thread.h"

#define LT_LOG(log_fmt, ...)                                        \
  lt_log_print(LOG_CONNECTION_FD, "winpoll: " log_fmt, __VA_ARGS__);

#define LT_LOG_EVENT(log_fmt, ...)                                      \
  lt_log_print(LOG_CONNECTION_FD, "winpoll->%i : %s : " log_fmt, event->file_descriptor(), event->type_name(), __VA_ARGS__);

namespace torrent::net {

namespace {
constexpr uint32_t kRead  = 0x1;
constexpr uint32_t kWrite = 0x2;
constexpr uint32_t kError = 0x4;

short
to_wsa_events(uint32_t mask) {
  short events = 0;

  if (mask & kRead)
    events |= POLLRDNORM;
  if (mask & kWrite)
    events |= POLLWRNORM;
  if (mask & kError)
    events |= POLLERR;

  return events;
}

bool
has_read_event(short revents) {
  return (revents & (POLLRDNORM | POLLIN)) != 0;
}

bool
has_write_event(short revents) {
  return (revents & (POLLWRNORM | POLLOUT)) != 0;
}

bool
has_error_event(short revents) {
  return (revents & (POLLERR | POLLHUP | POLLNVAL)) != 0;
}
} // namespace

class PollEvent {
public:
  PollEvent(Event* event) : event(event) {}
  ~PollEvent() = default;

  uint32_t            mask{};
  Event*              event{};
};

class PollInternal {
public:
  using Table = std::map<unsigned int, std::shared_ptr<PollEvent>>;

  uint32_t            event_mask(Event* event);
  void                set_event_mask(Event* event, uint32_t mask);
  void                modify(Event* event, uint32_t mask, uint32_t old_mask);

  unsigned int        m_max_sockets{};
  unsigned int        m_waiting_events{};
  int                 m_last_error{};

  Table                         m_table;
  std::vector<WSAPOLLFD>        m_poll_fds;
  std::vector<PollEvent*>       m_poll_events;
};

uint32_t
PollInternal::event_mask(Event* event) {
  if (event->file_descriptor() == -1)
    throw internal_error("PollInternal::event_mask() invalid file descriptor for event: " + event->print_name_fd_str());

  auto itr = m_table.find(event->file_descriptor());

  if (itr == m_table.end())
    throw internal_error("PollInternal::event_mask() event not found: " + event->print_name_fd_str());

  if (event != itr->second->event)
    throw internal_error("PollInternal::event_mask() event mismatch: " + event->print_name_fd_str());

  return itr->second->mask;
}

void
PollInternal::set_event_mask(Event* event, uint32_t mask) {
  if (event->file_descriptor() == -1)
    throw internal_error("PollInternal::set_event_mask() invalid file descriptor for event: " + event->print_name_fd_str());

  event->m_poll_event->mask = mask;
}

void
PollInternal::modify(Event* event, uint32_t mask, uint32_t old_mask) {
  if (old_mask == mask)
    return;

  if (event->m_poll_event == nullptr)
    throw internal_error("PollInternal::modify(): event has no PollEvent associated: " + event->print_name_fd_str());

  set_event_mask(event, mask);
}

std::unique_ptr<Poll>
Poll::create() {
  auto poll = new Poll();

  poll->m_internal                = std::make_unique<PollInternal>();
  poll->m_internal->m_max_sockets = 1024;

  return std::unique_ptr<Poll>(poll);
}

Poll::~Poll() {
  assert(m_internal->m_table.empty() && "Poll::~Poll() called with non-empty event table.");
}

unsigned int
Poll::do_poll(int64_t timeout_usec) {
  int status = poll(timeout_usec);

  if (status == -1) {
    if (m_internal->m_last_error == WSAEINTR)
      return 0;

    throw internal_error("Poll::work() WSA error: " + std::to_string(m_internal->m_last_error));
  }

  return process();
}

int
Poll::poll(int timeout_usec) {
  m_internal->m_poll_fds.clear();
  m_internal->m_poll_events.clear();

  m_internal->m_poll_fds.reserve(m_internal->m_table.size());
  m_internal->m_poll_events.reserve(m_internal->m_table.size());

  for (const auto& [fd, poll_event] : m_internal->m_table) {
    if (poll_event == nullptr || poll_event->event == nullptr)
      continue;
    if (poll_event->mask == 0)
      continue;

    WSAPOLLFD pfd{};
    pfd.fd = static_cast<SOCKET>(fd);
    pfd.events = to_wsa_events(poll_event->mask);

    m_internal->m_poll_fds.push_back(pfd);
    m_internal->m_poll_events.push_back(poll_event.get());
  }

  if (m_internal->m_poll_fds.empty()) {
    if (timeout_usec > 0)
      ::Sleep(static_cast<DWORD>(timeout_usec / 1000));
    return 0;
  }

  int timeout_ms = timeout_usec < 0 ? -1 : static_cast<int>(timeout_usec / 1000);
  int result = ::WSAPoll(m_internal->m_poll_fds.data(),
                         static_cast<ULONG>(m_internal->m_poll_fds.size()),
                         timeout_ms);

  if (result == SOCKET_ERROR) {
    m_internal->m_last_error = ::WSAGetLastError();
    return -1;
  }

  m_internal->m_last_error = 0;
  m_internal->m_waiting_events = result;
  return result;
}

unsigned int
Poll::process() {
  unsigned int count{};

  m_processing = true;
  m_closed_events.clear();

  for (size_t i = 0; i < m_internal->m_poll_fds.size(); ++i) {
    if (utils::Thread::self()->callbacks_should_interrupt_polling())
      utils::Thread::self()->process_callbacks(true);

    auto* poll_event = m_internal->m_poll_events[i];
    if (poll_event == nullptr || poll_event->event == nullptr)
      continue;

    short revents = m_internal->m_poll_fds[i].revents;
    if (revents == 0)
      continue;

    if (has_error_event(revents)) {
      count++;

      if (!(poll_event->mask & kError))
        throw internal_error("Poll::process() received error event for event not in error: " + poll_event->event->print_name_fd_str());

      auto event_info = poll_event->event->print_name_fd_str();

      poll_event->event->event_error();

      if (poll_event->mask != 0)
        throw internal_error("Poll::process() event_error called but event mask not cleared: " + event_info);

      continue;
    }

    if (has_read_event(revents) && (poll_event->mask & kRead)) {
      poll_event->event->event_read();
      count++;
    }

    if (has_write_event(revents) && (poll_event->mask & kWrite)) {
      poll_event->event->event_write();
      count++;
    }
  }

  m_closed_events.clear();
  m_processing = false;
  m_internal->m_waiting_events = 0;

  return count;
}

uint32_t
Poll::open_max() const {
  return m_internal->m_max_sockets;
}

void
Poll::open(Event* event) {
  LT_LOG_EVENT("open event", 0);

  if (event->file_descriptor() == -1)
    throw internal_error("Poll::open() invalid file descriptor for event: " + event->print_name_fd_str());

  if (event->m_poll_event != nullptr)
    throw internal_error("Poll::open() called but the event is already associated with a poll: " + event->print_name_fd_str());

  if (m_internal->m_table.find(event->file_descriptor()) != m_internal->m_table.end())
    throw internal_error("Poll::open() event already exists: " + event->print_name_fd_str());

  event->m_poll_event = std::make_shared<PollEvent>(event);
  m_internal->m_table[event->file_descriptor()] = event->m_poll_event;
}

void
Poll::close(Event* event) {
  LT_LOG_EVENT("close event", 0);

  auto* poll_event = event->m_poll_event.get();

  if (poll_event == nullptr)
    return;

  if (poll_event->event != event)
    throw internal_error("Poll::close() event mismatch: " + event->print_name_fd_str());

  if (m_internal->event_mask(event) != 0)
    throw internal_error("Poll::close() called but the file descriptor is active: " + event->print_name_fd_str());

  if (m_internal->m_table.erase(event->file_descriptor()) == 0)
    throw internal_error("Poll::close() event not found: " + event->print_name_fd_str());

  if (m_processing)
    m_closed_events.push_back(event->m_poll_event);

  poll_event->event   = nullptr;
  event->m_poll_event = nullptr;
}

bool
Poll::in_read(Event* event) {
  return m_internal->event_mask(event) & kRead;
}

bool
Poll::in_write(Event* event) {
  return m_internal->event_mask(event) & kWrite;
}

bool
Poll::in_error(Event* event) {
  return m_internal->event_mask(event) & kError;
}

void
Poll::insert_read(Event* event) {
  auto mask = m_internal->event_mask(event);

  if (mask & kRead)
    return;

  LT_LOG_EVENT("insert read", 0);

  m_internal->modify(event, mask | kRead, mask);
}

void
Poll::insert_write(Event* event) {
  auto mask = m_internal->event_mask(event);

  if (mask & kWrite)
    return;

  LT_LOG_EVENT("insert write", 0);

  m_internal->modify(event, mask | kWrite, mask);
}

void
Poll::insert_error(Event* event) {
  auto mask = m_internal->event_mask(event);

  if (mask & kError)
    return;

  LT_LOG_EVENT("insert error", 0);

  m_internal->modify(event, mask | kError, mask);
}

void
Poll::remove_read(Event* event) {
  auto mask     = m_internal->event_mask(event);
  auto new_mask = mask & ~kRead;

  if (!(mask & kRead))
    return;

  LT_LOG_EVENT("remove read", 0);

  m_internal->modify(event, new_mask, mask);
}

void
Poll::remove_write(Event* event) {
  auto mask     = m_internal->event_mask(event);
  auto new_mask = mask & ~kWrite;

  if (!(mask & kWrite))
    return;

  LT_LOG_EVENT("remove write", 0);

  m_internal->modify(event, new_mask, mask);
}

void
Poll::remove_error(Event* event) {
  auto mask     = m_internal->event_mask(event);
  auto new_mask = mask & ~kError;

  if (!(mask & kError))
    return;

  LT_LOG_EVENT("remove error", 0);

  m_internal->modify(event, new_mask, mask);
}

void
Poll::remove_and_close(Event* event) {
  remove_read(event);
  remove_write(event);
  remove_error(event);

  close(event);
}

} // namespace torrent::net

#endif // WINDOWS
