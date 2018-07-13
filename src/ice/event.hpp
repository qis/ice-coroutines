#pragma once
#include <ice/config.hpp>
#include <ice/error.hpp>
#include <ice/service.hpp>
#include <atomic>
#include <experimental/coroutine>
#include <system_error>

#if ICE_OS_WIN32
#  include <windows.h>
#elif ICE_OS_LINUX
#  include <sys/epoll.h>
#  define ICE_EVENT_RECV EPOLLIN
#  define ICE_EVENT_SEND EPOLLOUT
#elif ICE_OS_FREEBSD
#  include <sys/event.h>
#  define ICE_EVENT_RECV EVFILT_READ
#  define ICE_EVENT_SEND EVFILT_WRITE
#endif

namespace ice {

#if ICE_OS_WIN32

class event final : public OVERLAPPED {
public:
  event() noexcept : OVERLAPPED({}) {}

  // clang-format off
#ifdef __INTELLISENSE__
  event(event&& other) {}
  event(const event& other) {}
  event& operator=(event&& other) { return *this; }
  event& operator=(const event& other) { return *this; }
#else
  event(event&& other) = delete;
  event(const event& other) = delete;
  event& operator=(event&& other) = delete;
  event& operator=(const event& other) = delete;
#endif
  // clang-format on

  ~event() = default;

  constexpr bool await_ready() const noexcept
  {
    return false;
  }

  void await_suspend(std::experimental::coroutine_handle<> awaiter) noexcept
  {
    awaiter_ = awaiter;
    if (ready_.exchange(true, std::memory_order_acq_rel)) {
      awaiter_.resume();
    }
  }

  constexpr void await_resume() const noexcept {}

  void resume() noexcept
  {
    if (ready_.exchange(true, std::memory_order_acq_rel)) {
      awaiter_.resume();
    }
  }

private:
  std::atomic_bool ready_{ false };
  std::experimental::coroutine_handle<> awaiter_;
};

#elif ICE_OS_LINUX

class event {
public:
  event(service& service, int handle, uint32_t events) noexcept :
    service_(service.handle()), handle_(handle), events_(events)
  {}

  event(event&& other) = delete;
  event(const event& other) = delete;
  event& operator=(event&& other) = delete;
  event& operator=(const event& other) = delete;

  ~event() = default;

  constexpr bool await_ready() const noexcept
  {
    return false;
  }

  bool await_suspend(std::experimental::coroutine_handle<> awaiter) noexcept
  {
    awaiter_ = awaiter;
    struct epoll_event nev;
    nev.events = events_ | EPOLLONESHOT;
    nev.data.ptr = this;
    if (::epoll_ctl(service_, EPOLL_CTL_ADD, handle_, &nev) < 0) {
      ec_ = make_error_code(errno);
      return false;
    }
    return true;
  }

  std::error_code await_resume() const noexcept
  {
    return ec_;
  }

  void resume() noexcept
  {
    struct epoll_event nev;
    nev.events = events_ | EPOLLONESHOT;
    nev.data.ptr = this;
    if (::epoll_ctl(service_, EPOLL_CTL_DEL, handle_, &nev) < 0) {
      ec_ = make_error_code(errno);
    }
    awaiter_.resume();
  }

private:
  std::experimental::coroutine_handle<> awaiter_;
  std::error_code ec_;
  int service_ = -1;
  int handle_ = -1;
  uint32_t events_ = 0;
};

#elif ICE_OS_FREEBSD

class event {
public:
  event(service& service, int handle, short filter) noexcept :
    service_(service.handle()), handle_(handle), filter_(filter)
  {}

  event(event&& other) = delete;
  event(const event& other) = delete;
  event& operator=(event&& other) = delete;
  event& operator=(const event& other) = delete;

  ~event() = default;

  constexpr bool await_ready() const noexcept
  {
    return false;
  }

  bool await_suspend(std::experimental::coroutine_handle<> awaiter) noexcept
  {
    awaiter_ = awaiter;
    struct kevent nev;
    EV_SET(&nev, static_cast<uintptr_t>(handle_), filter_, EV_ADD | EV_ONESHOT, 0, 0, this);
    if (::kevent(service_, &nev, 1, nullptr, 0, nullptr) < 0) {
      ec_ = make_error_code(errno);
      return false;
    }
    return true;
  }

  std::error_code await_resume() const noexcept
  {
    return ec_;
  }

  void resume() noexcept
  {
    awaiter_.resume();
  }

private:
  std::experimental::coroutine_handle<> awaiter_;
  std::error_code ec_;
  int service_ = -1;
  int handle_ = -1;
  short filter_ = 0;
};

#endif

}  // namespace ice
