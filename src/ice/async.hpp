#pragma once
#include <ice/config.hpp>
#include <atomic>
#include <condition_variable>
#include <experimental/coroutine>
#include <mutex>

#if ICE_EXCEPTIONS
#  include <exception>
#endif

namespace ice {

struct task {
  struct promise_type {
    constexpr task get_return_object() const noexcept
    {
      return {};
    }

    constexpr auto initial_suspend() const noexcept
    {
      return std::experimental::suspend_never{};
    }

    constexpr auto final_suspend() const noexcept
    {
      return std::experimental::suspend_never{};
    }

    constexpr void return_void() const noexcept {}

#if ICE_EXCEPTIONS || defined(__clang__)
    void unhandled_exception() const noexcept(ICE_NO_EXCEPTIONS)
    {
#  if ICE_EXCEPTIONS
      throw;
#  endif
    }
#endif
  };
};

struct sync {
  struct state {
    std::atomic_bool ready = false;
    std::condition_variable cv;
    std::mutex mutex;
#if ICE_EXCEPTIONS
    std::exception_ptr exception;
#endif
  };

  struct promise_type {
    sync get_return_object() noexcept
    {
      return { this };
    }

    constexpr auto initial_suspend() const noexcept
    {
      return std::experimental::suspend_never{};
    }

    constexpr auto final_suspend() const noexcept
    {
      return std::experimental::suspend_never{};
    }

    void return_void() noexcept
    {
      state_->ready.store(true, std::memory_order_release);
      state_->cv.notify_one();
    }

#if ICE_EXCEPTIONS || defined(__clang__)
    void unhandled_exception() const noexcept(ICE_NO_EXCEPTIONS)
    {
#  if ICE_EXCEPTIONS
      state_->exception = std::current_exception();
#  endif
    }
#endif

    state* state_ = nullptr;
  };

  sync(promise_type* promise) noexcept(ICE_NO_EXCEPTIONS) : state_(std::make_unique<state>())
  {
    promise->state_ = state_.get();
  }

  sync(sync&& other) = default;
  sync(const sync& other) = delete;
  sync& operator=(sync&& other) = default;
  sync& operator=(const sync& other) = delete;

  ~sync() noexcept(ICE_NO_EXCEPTIONS)
  {
    if (auto state = join()) {
#if ICE_EXCEPTIONS
      if (state_->exception && std::uncaught_exceptions() == 0) {
        std::rethrow_exception(state_->exception);
      }
#endif
    }
  }

  void get() noexcept(ICE_NO_EXCEPTIONS)
  {
    if (auto state = join()) {
#if ICE_EXCEPTIONS
      if (state_->exception) {
        std::rethrow_exception(state_->exception);
      }
#endif
    }
  }

  std::unique_ptr<state> join() noexcept
  {
    if (auto state = std::exchange(state_, nullptr)) {
      std::unique_lock lock{ state_->mutex };
      state_->cv.wait(lock, [this]() { return state_->ready.load(std::memory_order_acquire); });
      return state;
    }
    return {};
  }

private:
  std::unique_ptr<state> state_;
};

}  // namespace ice
