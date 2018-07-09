#pragma once
#include <ice/config.hpp>
#include <experimental/coroutine>

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

}  // namespace ice
