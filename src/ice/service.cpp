#include "service.hpp"
#include <ice/error.hpp>
#include <vector>
#include <cassert>

#if ICE_OS_WIN32
#  include <windows.h>
#  include <winsock2.h>
#else
#  if ICE_OS_LINUX
#    include <sys/epoll.h>
#    include <sys/eventfd.h>
#  elif ICE_OS_FREEBSD
#    include <sys/event.h>
#  endif
#  include <unistd.h>
#endif

namespace ice {

#if ICE_OS_WIN32
native_event::native_event() noexcept
{
  new (get()) OVERLAPPED{};
}

native_event::~native_event()
{
  get()->~OVERLAPPED();
}
#endif

#if ICE_OS_WIN32
void service::close_type::operator()(std::uintptr_t handle) noexcept
{
  ::CloseHandle(reinterpret_cast<HANDLE>(handle));
}
#else
void service::close_type::operator()(int handle) noexcept
{
  ::close(handle);
}
#endif

service::service(std::size_t concurrency_hint)
{
#if ICE_OS_WIN32
  struct wsa {
    wsa() noexcept
    {
      WSADATA wsadata = {};
      if (const auto rc = ::WSAStartup(MAKEWORD(2, 2), &wsadata)) {
        ec = make_error_code(rc);
      }
      const auto major = LOBYTE(wsadata.wVersion);
      const auto minor = HIBYTE(wsadata.wVersion);
      if (major < 2 || (major == 2 && minor < 2)) {
        ec = make_error_code(errc::version_mismatch);
      }
    }
    std::error_code ec;
  };
  static const wsa wsa;
  if (wsa.ec) {
    throw_error(wsa.ec, "wsa startup");
    return;
  }
  handle_type handle(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, static_cast<DWORD>(concurrency_hint)));
  if (!handle) {
    throw_error(::GetLastError(), "create completion port");
    return;
  }
#elif ICE_OS_LINUX
  (void)concurrency_hint;
  handle_type handle(::epoll_create1(0));
  if (!handle) {
    throw_error(errno, "create epoll");
    return;
  }
  handle_type events(::eventfd(0, EFD_NONBLOCK));
  if (!events) {
    throw_error(errno, "create eventfd");
    return;
  }
  epoll_event nev = { EPOLLONESHOT, {} };
  if (::epoll_ctl(handle, EPOLL_CTL_ADD, events, &nev) < 0) {
    throw_error(errno, "add eventfd to epoll");
    return;
  }
  events_ = std::move(events);
#elif ICE_OS_FREEBSD
  (void)concurrency_hint;
  handle_type handle(::kqueue());
  if (!handle) {
    throw_error(errno, "create kqueue");
    return;
  }
  struct kevent nev = {};
  EV_SET(&nev, 0, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);
  if (::kevent(handle, &nev, 1, nullptr, 0, nullptr) < 0) {
    throw_error(errno, "add notification event to kqueue");
    return;
  }
#endif
  handle_ = std::move(handle);
  interrupt();
}

void service::run()
{
  run(128);
}

void service::run(std::error_code& ec) noexcept
{
  run(128, ec);
}

void service::run(std::size_t event_buffer_size)
{
  std::error_code ec;
  run(event_buffer_size, ec);
  throw_on_error(ec, "run service");
}

void service::run(std::size_t event_buffer_size, std::error_code& ec) noexcept
{
#if ICE_OS_WIN32
  using data_type = OVERLAPPED_ENTRY;
  using size_type = ULONG;
#elif ICE_OS_LINUX
  using data_type = struct epoll_event;
  using size_type = int;
#elif ICE_OS_FREEBSD
  using data_type = struct kevent;
  using size_type = int;
#endif

  std::vector<data_type> events;
  events.resize(event_buffer_size);

  const auto index = index_.set(this);
  const auto events_data = events.data();
  const auto events_size = static_cast<size_type>(events.size());

  while (true) {
#if ICE_OS_WIN32
    size_type count = 0;
    if (!::GetQueuedCompletionStatusEx(handle_.as<HANDLE>(), events_data, events_size, &count, INFINITE, FALSE)) {
      if (const auto rc = ::GetLastError(); rc != ERROR_ABANDONED_WAIT_0) {
        ec = make_error_code(rc);
        return;
      }
      break;
    }
#elif ICE_OS_LINUX
    const auto count = ::epoll_wait(handle_, events_data, events_size, -1);
    if (count < 0 && errno != EINTR) {
      ec = make_error_code(errno);
      return;
    }
#elif ICE_OS_FREEBSD
    const auto count = ::kevent(handle_, nullptr, 0, events_data, events_size, nullptr);
    if (count < 0 && errno != EINTR) {
      ec = make_error_code(errno);
      return;
    }
#endif
    auto stop = false;
    auto interrupted = false;
    for (size_type i = 0; i < count; i++) {
      auto& entry = events_data[i];
#if ICE_OS_WIN32
      if (const auto ev = static_cast<native_event*>(reinterpret_cast<native_event_storage*>(entry.lpOverlapped))) {
        ev->resume();
        continue;
      }
#elif ICE_OS_LINUX
      if (const auto ev = reinterpret_cast<native_event*>(entry.data.ptr)) {
        if (ev->native_handle_ != -1) {
          ::epoll_ctl(handle_, EPOLL_CTL_DEL, ev->native_handle_, &entry);
          ev->native_handle_ = -1;
        }
        ev->resume();
        continue;
      }
#elif ICE_OS_FREEBSD
      if (const auto ev = reinterpret_cast<native_event*>(entry.udata)) {
        ev->resume();
        continue;
      }
#endif
      interrupted = true;
      stop = stop_.load(std::memory_order_acquire);
    }
    if (interrupted) {
      process();
    }
    if (stop) {
      break;
    }
  }
}

#if ICE_OS_LINUX

bool service::queue_recv(int handle, native_event* ev) noexcept
{
  assert(ev);
  assert(ev->native_handle_ == -1);
  ev->native_handle_ = handle;
  struct epoll_event nev {
    EPOLLIN | EPOLLONESHOT, {}
  };
  nev.data.ptr = ev;
  if (::epoll_ctl(handle_, EPOLL_CTL_ADD, handle, &nev) < 0) {
    ev->ec_ = make_error_code(errno);
    return false;
  }
  return true;
}

bool service::queue_send(int handle, native_event* ev) noexcept
{
  assert(ev);
  assert(ev->native_handle_ == -1);
  ev->native_handle_ = handle;
  struct epoll_event nev {
    EPOLLOUT | EPOLLONESHOT, {}
  };
  nev.data.ptr = ev;
  if (::epoll_ctl(handle_, EPOLL_CTL_ADD, handle, &nev) < 0) {
    ev->ec_ = make_error_code(errno);
    return false;
  }
  return true;
}

#elif ICE_OS_FREEBSD

bool service::queue_recv(int handle, native_event* ev) noexcept
{
  assert(ev);
  struct kevent nev;
  EV_SET(&nev, static_cast<uintptr_t>(handle), EVFILT_READ, EV_ADD | EV_ONESHOT, 0, 0, ev);
  if (::kevent(handle_, &nev, 1, nullptr, 0, nullptr) < 0) {
    ev->ec_ = make_error_code(errno);
    return false;
  }
  return true;
}

bool service::queue_send(int handle, native_event* ev) noexcept
{
  assert(ev);
  struct kevent nev;
  EV_SET(&nev, static_cast<uintptr_t>(handle), EVFILT_WRITE, EV_ADD | EV_ONESHOT, 0, 0, ev);
  if (::kevent(handle_, &nev, 1, nullptr, 0, nullptr) < 0) {
    ev->ec_ = make_error_code(errno);
    return false;
  }
  return true;
}

#endif

void service::interrupt() noexcept
{
#if ICE_OS_WIN32
  ::PostQueuedCompletionStatus(handle_.as<HANDLE>(), 0, 0, nullptr);
#elif ICE_OS_LINUX
  epoll_event nev{ EPOLLOUT | EPOLLONESHOT, {} };
  ::epoll_ctl(handle_, EPOLL_CTL_MOD, events_, &nev);
#elif ICE_OS_FREEBSD
  struct kevent nev {};
  EV_SET(&nev, 0, EVFILT_USER, 0, NOTE_TRIGGER, 0, nullptr);
  ::kevent(handle_, &nev, 1, nullptr, 0, nullptr);
#endif
}

}  // namespace ice
