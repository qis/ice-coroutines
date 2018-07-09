#pragma once
#include <ice/config.hpp>
#include <ice/scheduler.hpp>
#include <ice/utility.hpp>
#include <system_error>
#include <type_traits>

#if ICE_OS_WIN32
typedef struct _OVERLAPPED OVERLAPPED;
#endif

namespace ice {

struct native_event_storage {
  std::aligned_storage_t<native_event_size, native_event_alignment> storage;
};

class native_event
#if ICE_OS_WIN32
  : public native_event_storage
#endif
{
  friend class service;
public:
#if ICE_OS_WIN32
  native_event() noexcept;
#else
  native_event() noexcept = default;
#endif

  // clang-format off
#ifdef __INTELLISENSE__
  native_event(native_event&& other) {}
  native_event(const native_event& other) {}
  native_event& operator=(native_event&& other) { return *this; }
  native_event& operator=(const native_event& other) { return *this; }
#else
  native_event(native_event&& other) = delete;
  native_event(const native_event& other) = delete;
  native_event& operator=(native_event&& other) = delete;
  native_event& operator=(const native_event& other) = delete;
#endif
  // clang-format on

#if ICE_OS_WIN32
  virtual ~native_event();
#else
  virtual ~native_event() = default;
#endif

  bool await_suspend(std::experimental::coroutine_handle<> awaiter) noexcept
  {
    awaiter_ = awaiter;
    return native_suspend();
  }

  void resume() noexcept
  {
    if (native_resume() || !native_suspend()) {
      awaiter_.resume();
    }
  }

  virtual bool native_suspend() noexcept = 0;
  virtual bool native_resume() noexcept = 0;

#if ICE_OS_WIN32
  OVERLAPPED* get() noexcept
  {
    return reinterpret_cast<OVERLAPPED*>(static_cast<native_event_storage*>(this));
  }
#endif

protected:
  std::error_code ec_;

private:
  std::experimental::coroutine_handle<> awaiter_;
#if ICE_OS_LINUX
  int native_handle_ = -1;
#endif
};

class service final : private scheduler {
public:
#if ICE_OS_WIN32
  struct close_type {
    void operator()(std::uintptr_t handle) noexcept;
  };
  using handle_type = ice::handle<std::uintptr_t, 0, close_type>;
#else
  struct close_type {
    void operator()(int handle) noexcept;
  };
  using handle_type = ice::handle<int, -1, close_type>;
#endif
  using handle_view = handle_type::view;

  service();

  void run(std::size_t event_buffer_size = 128);

  bool is_current() const noexcept
  {
    return index_.get() ? true : false;
  }

  void stop() noexcept
  {
    stop_.store(true, std::memory_order_release);
    interrupt();
  }

  void post(event* ev) noexcept
  {
    scheduler::post(ev);
    interrupt();
  }

  ice::schedule<service> schedule(bool post = false) noexcept
  {
    return { *this, post };
  }

  handle_view handle() const noexcept
  {
    return handle_;
  }

#if ICE_OS_LINUX
  handle_view events() const noexcept
  {
    return events_;
  }
#endif

#if ICE_OS_LINUX || ICE_OS_FREEBSD
  bool queue_recv(int handle, native_event* ev) noexcept;
  bool queue_send(int handle, native_event* ev) noexcept;
#endif

private:
  void interrupt() noexcept;

  std::atomic_bool stop_ = false;
  ice::thread_local_storage index_;
  handle_type handle_;
#if ICE_OS_LINUX
  handle_type events_;
#endif
};

}  // namespace ice
