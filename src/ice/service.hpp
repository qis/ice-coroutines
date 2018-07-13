#pragma once
#include <ice/config.hpp>
#include <ice/scheduler.hpp>
#include <ice/utility.hpp>
#include <system_error>

namespace ice {

class service final : public scheduler<service> {
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

  std::error_code create() noexcept;
  std::error_code run(std::size_t event_buffer_size = 128) noexcept;

  bool is_current() const noexcept
  {
    return index_.get() ? true : false;
  }

  std::error_code stop() noexcept
  {
    stop_.store(true, std::memory_order_release);
    return interrupt();
  }

  std::error_code post(ice::schedule<service>* schedule) noexcept
  {
    scheduler::post(schedule);
    return interrupt();
  }

  constexpr handle_type& handle() noexcept
  {
    return handle_;
  }

  constexpr const handle_type& handle() const noexcept
  {
    return handle_;
  }

#if ICE_OS_LINUX
  constexpr handle_type& events() noexcept
  {
    return events_;
  }

  constexpr const handle_type& events() const noexcept
  {
    return events_;
  }
#endif

private:
  std::error_code interrupt() noexcept;

  std::atomic_bool stop_ = false;
  ice::thread_local_storage index_;
  handle_type handle_;
#if ICE_OS_LINUX
  handle_type events_;
#endif
};

}  // namespace ice
