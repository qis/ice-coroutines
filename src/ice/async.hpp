#pragma once
#include <ice/config.hpp>
#include <ice/log.hpp>
#include <atomic>
#include <condition_variable>
#include <experimental/coroutine>
#include <mutex>

#if ICE_EXCEPTIONS
#  include <exception>
#endif

namespace ice {
namespace detail {

struct task_promise {
  constexpr auto initial_suspend() const noexcept
  {
    return std::experimental::suspend_never{};
  }

  constexpr auto final_suspend() const noexcept
  {
    return std::experimental::suspend_never{};
  }

#if ICE_EXCEPTIONS || defined(__clang__)
  void unhandled_exception() const noexcept(ICE_NO_EXCEPTIONS)
  {
#  if ICE_EXCEPTIONS
#    if ICE_DEBUG
    throw;
#    else
    try {
      throw;
    }
    catch (const std::system_error& e) {
      ice::log::error(
        "{} error {}: {} ({})", e.code().category().name(), e.code().value(), e.what(), e.code().message());
    }
    catch (const std::exception& e) {
      ice::log::error(e.what());
    }
    catch (...) {
      ice::log::error("unhandled exception");
    }
#    endif
#  endif
  }
#endif
};

}  // namespace detail

struct task {
  struct promise_type final : public detail::task_promise {
    constexpr task get_return_object() const noexcept
    {
      return {};
    }

    constexpr void return_void() const noexcept {}
  };
};

struct sync {
  struct state {
    std::atomic_bool ready = false;
    std::condition_variable cv;
    std::mutex mutex;
  };

  struct promise_type final : public detail::task_promise {
    sync get_return_object() noexcept
    {
      return { this };
    }

    void return_void() noexcept
    {
      state_->ready.store(true, std::memory_order_release);
      state_->cv.notify_one();
    }

    state* state_ = nullptr;
  };

  sync(promise_type* promise) noexcept(ICE_NO_EXCEPTIONS) : state_(std::make_unique<state>())
  {
    promise->state_ = state_.get();
  }

  sync(sync&& other) = default;
  sync& operator=(sync&& other) = default;

  ~sync()
  {
    std::unique_lock lock{ state_->mutex };
    state_->cv.wait(lock, [this]() { return state_->ready.load(std::memory_order_acquire); });
  }

private:
  std::unique_ptr<state> state_;
};

}  // namespace ice
