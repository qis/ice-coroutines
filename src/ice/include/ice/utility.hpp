#pragma once
#include <ice/config.hpp>
#include <ice/handle.hpp>
#include <system_error>
#include <type_traits>
#include <utility>
#include <cstdint>

#ifndef ICE_LIKELY
#  ifdef __has_builtin
#    if __has_builtin(__builtin_expect)
#      define ICE_LIKELY(expr) __builtin_expect(expr, 1)
#    endif
#  endif
#endif
#ifndef ICE_LIKELY
#  define ICE_LIKELY(expr) expr
#endif

#ifndef ICE_UNLIKELY
#  ifdef __has_builtin
#    if __has_builtin(__builtin_expect)
#      define ICE_UNLIKELY(expr) __builtin_expect(expr, 0)
#    endif
#  endif
#endif
#ifndef ICE_UNLIKELY
#  define ICE_UNLIKELY(expr) expr
#endif

#ifndef ICE_ALWAYS_INLINE
#  if defined(_MSC_VER) && !defined(__clang__)
#    define ICE_ALWAYS_INLINE __forceinline
#  else
#    define ICE_ALWAYS_INLINE __attribute__((always_inline))
#  endif
#endif

#ifndef ICE_OFFSETOF
#  if defined(_MSC_VER) && !defined(__clang__)
#    define ICE_OFFSETOF(s, m) ((size_t)(&reinterpret_cast<char const volatile&>((((s*)0)->m))))
#  else
#    define ICE_OFFSETOF __builtin_offsetof
#  endif
#endif

namespace ice {

template <typename T, bool = std::is_move_constructible_v<T>>
struct delete_move_constructor {
  delete_move_constructor() = default;
  delete_move_constructor(delete_move_constructor&& other) = default;
  delete_move_constructor(const delete_move_constructor& other) = default;
  delete_move_constructor& operator=(delete_move_constructor&& other) = default;
  delete_move_constructor& operator=(const delete_move_constructor& other) = default;
};

template <typename T>
struct delete_move_constructor<T, false> {
  delete_move_constructor() = default;
  delete_move_constructor(delete_move_constructor&& other) = delete;
  delete_move_constructor(const delete_move_constructor& other) = default;
  delete_move_constructor& operator=(delete_move_constructor&& other) = default;
  delete_move_constructor& operator=(const delete_move_constructor& other) = default;
};

template <typename T, bool = std::is_copy_constructible_v<T>>
struct delete_copy_constructor {
  delete_copy_constructor() = default;
  delete_copy_constructor(delete_copy_constructor&& other) = default;
  delete_copy_constructor(const delete_copy_constructor& other) = default;
  delete_copy_constructor& operator=(delete_copy_constructor&& other) = default;
  delete_copy_constructor& operator=(const delete_copy_constructor& other) = default;
};

template <typename T>
struct delete_copy_constructor<T, false> {
  delete_copy_constructor() = default;
  delete_copy_constructor(delete_copy_constructor&& other) = default;
  delete_copy_constructor(const delete_copy_constructor& other) = delete;
  delete_copy_constructor& operator=(delete_copy_constructor&& other) = default;
  delete_copy_constructor& operator=(const delete_copy_constructor& other) = default;
};

template <typename T, bool = std::is_move_assignable_v<T>>
struct delete_move_assignment {
  delete_move_assignment() = default;
  delete_move_assignment(delete_move_assignment&& other) = default;
  delete_move_assignment(const delete_move_assignment& other) = default;
  delete_move_assignment& operator=(delete_move_assignment&& other) = default;
  delete_move_assignment& operator=(const delete_move_assignment& other) = default;
};

template <typename T>
struct delete_move_assignment<T, false> {
  delete_move_assignment() = default;
  delete_move_assignment(delete_move_assignment&& other) = default;
  delete_move_assignment(const delete_move_assignment& other) = default;
  delete_move_assignment& operator=(delete_move_assignment&& other) = delete;
  delete_move_assignment& operator=(const delete_move_assignment& other) = default;
};

template <typename T, bool = std::is_copy_assignable_v<T>>
struct delete_copy_assignment {
  delete_copy_assignment() = default;
  delete_copy_assignment(delete_copy_assignment&& other) = default;
  delete_copy_assignment(const delete_copy_assignment& other) = default;
  delete_copy_assignment& operator=(delete_copy_assignment&& other) = default;
  delete_copy_assignment& operator=(const delete_copy_assignment& other) = default;
};

template <typename T>
struct delete_copy_assignment<T, false> {
  delete_copy_assignment() = default;
  delete_copy_assignment(delete_copy_assignment&& other) = default;
  delete_copy_assignment(const delete_copy_assignment& other) = default;
  delete_copy_assignment& operator=(delete_copy_assignment&& other) = default;
  delete_copy_assignment& operator=(const delete_copy_assignment& other) = delete;
};

template <typename Handler>
class scope_exit {
public:
  explicit scope_exit(Handler handler) noexcept : handler_(std::move(handler)) {}

  scope_exit(scope_exit&& other) noexcept :
    handler_(std::move(other.handler_)), invoke_(std::exchange(other.invoke_, false))
  {}

  scope_exit(const scope_exit& other) = delete;
  scope_exit& operator=(const scope_exit& other) = delete;

  ~scope_exit() noexcept(ICE_NO_EXCEPTIONS || noexcept(handler_()))
  {
    if (invoke_) {
      handler_();
    }
  }

private:
  Handler handler_;
  bool invoke_ = true;
};

template <typename Handler>
inline auto on_scope_exit(Handler&& handler) noexcept
{
  return scope_exit<Handler>{ std::forward<Handler>(handler) };
}

class thread_local_storage {
public:
  struct close_type {
    void operator()(std::uintptr_t handle) noexcept;
  };
#if ICE_OS_WIN32
  using handle_type = handle<std::uintptr_t, 0, close_type>;
#else
  using handle_type = handle<std::uintptr_t, -1, close_type>;
#endif
  using handle_view = handle_type::view;

  class lock {
  public:
    lock(handle_view handle, void* value) noexcept;

    lock(lock&& other) noexcept = default;
    lock& operator=(lock&& other) noexcept = default;

    ~lock();

  private:
    handle_view handle_;
  };

  thread_local_storage() noexcept;

  lock set(void* value) noexcept
  {
    return { handle_, value };
  }

  void* get() noexcept;
  const void* get() const noexcept;

private:
  handle_type handle_;
};

std::error_code set_thread_affinity(std::size_t index) noexcept;

}  // namespace ice
