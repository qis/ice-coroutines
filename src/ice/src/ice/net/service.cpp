#include "ice/net/service.hpp"
#include <ice/error.hpp>
#include <ice/net/event.hpp>
#include <vector>
#include <cassert>
#include <ctime>

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

#include <ice/log.hpp>

namespace ice::net {

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

std::error_code service::create() noexcept
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
    return wsa.ec;
  }
  handle_type handle(::CreateIoCompletionPort(INVALID_HANDLE_VALUE, nullptr, 0, 1));
  if (!handle) {
    return make_error_code(::GetLastError());
  }
#elif ICE_OS_LINUX
  handle_type handle(::epoll_create1(0));
  if (!handle) {
    return make_error_code(errno);
  }
  handle_type events(::eventfd(0, EFD_NONBLOCK));
  if (!events) {
    return make_error_code(errno);
  }
  epoll_event nev = { EPOLLONESHOT, {} };
  if (::epoll_ctl(handle, EPOLL_CTL_ADD, events, &nev) < 0) {
    return make_error_code(errno);
  }
  events_ = std::move(events);
#elif ICE_OS_FREEBSD
  handle_type handle(::kqueue());
  if (!handle) {
    return make_error_code(errno);
  }
  struct kevent nev = {};
  EV_SET(&nev, 0, EVFILT_USER, EV_ADD | EV_CLEAR, 0, 0, nullptr);
  if (::kevent(handle, &nev, 1, nullptr, 0, nullptr) < 0) {
    return make_error_code(errno);
  }
#endif
  handle_ = std::move(handle);
  return interrupt();
}

std::error_code service::run(std::size_t event_buffer_size) noexcept
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
  auto stop = stop_.load(std::memory_order_acquire);

#if ICE_OS_FREEBSD
  const timespec ts = { 0, 1000000 };
#endif

  while (true) {
#if ICE_OS_WIN32
    size_type count = 0;
    const DWORD timeout = stop ? 1 : INFINITE;
    if (!::GetQueuedCompletionStatusEx(handle_.as<HANDLE>(), events_data, events_size, &count, timeout, FALSE)) {
      if (const auto rc = ::GetLastError(); rc != ERROR_ABANDONED_WAIT_0 && rc != WAIT_TIMEOUT) {
        return make_error_code(rc);
      }
      break;
    }
#else
#  if ICE_OS_LINUX
    const auto timeout = stop ? 1 : -1;
    const auto count = ::epoll_wait(handle_, events_data, events_size, timeout);
#  elif ICE_OS_FREEBSD
    const auto timeout = stop ? &ts : nullptr;
    const auto count = ::kevent(handle_, nullptr, 0, events_data, events_size, timeout);
#  endif
    if (count <= 0) {
      if (count < 0 && errno != EINTR) {
        return make_error_code(errno);
      }
      if (stop_.load(std::memory_order_acquire)) {
        break;
      }
    }
#endif
    auto interrupted = false;
    for (size_type i = 0; i < count; i++) {
      auto& entry = events_data[i];
#if ICE_OS_WIN32
      if (const auto ev = static_cast<event*>(entry.lpOverlapped)) {
        ev->resume();
        continue;
      }
#elif ICE_OS_LINUX
      if (const auto ev = reinterpret_cast<event*>(entry.data.ptr)) {
        ev->resume();
        continue;
      }
#elif ICE_OS_FREEBSD
      if (const auto ev = reinterpret_cast<event*>(entry.udata)) {
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
  }
  return {};
}

std::error_code service::interrupt() noexcept
{
#if ICE_OS_WIN32
  if (!::PostQueuedCompletionStatus(handle_.as<HANDLE>(), 0, 0, nullptr)) {
    return make_error_code(::GetLastError());
  }
#elif ICE_OS_LINUX
  epoll_event nev{ EPOLLOUT | EPOLLONESHOT, {} };
  if (::epoll_ctl(handle_, EPOLL_CTL_MOD, events_, &nev) < 0) {
    return make_error_code(errno);
  }
#elif ICE_OS_FREEBSD
  struct kevent nev {};
  EV_SET(&nev, 0, EVFILT_USER, 0, NOTE_TRIGGER, 0, nullptr);
  if (::kevent(handle_, &nev, 1, nullptr, 0, nullptr) < 0) {
    return make_error_code(errno);
  }
#endif
  return {};
}

}  // namespace ice::net
